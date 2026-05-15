// Copyright (c) 2026 Terry Tian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <optional>
#include <string>

#include "minmea.h"
#include "libgnss/nmea_reader.hpp"

#include <cmath>

namespace
{

using namespace libgnss::nmea;
using namespace std::chrono;
using DispatchFn = std::optional<Sentence> (*)(const char*);

template <typename MinmeaSentence, typename Wrapper, bool (*Fn)(MinmeaSentence*, const char*)>
std::optional<Sentence> parseTyped(const char* s)
{
  MinmeaSentence frame{};
  if (!Fn(&frame, s))
  {
    return std::nullopt;
  }
  return Sentence{std::in_place_type<Wrapper>, frame};
}

// Index must match enum minmea_sentence_id values:
// 0 = UNKNOWN, 1 = GBS, 2 = GGA, ...
constexpr DispatchFn dispatch_table[] = {
  nullptr,  // MINMEA_UNKNOWN
  &parseTyped<minmea_sentence_gbs, SentenceGBS, minmea_parse_gbs>,
  &parseTyped<minmea_sentence_gga, SentenceGGA, minmea_parse_gga>,
  &parseTyped<minmea_sentence_gll, SentenceGLL, minmea_parse_gll>,
  &parseTyped<minmea_sentence_gsa, SentenceGSA, minmea_parse_gsa>,
  &parseTyped<minmea_sentence_gst, SentenceGST, minmea_parse_gst>,
  &parseTyped<minmea_sentence_gsv, SentenceGSV, minmea_parse_gsv>,
  &parseTyped<minmea_sentence_rmc, SentenceRMC, minmea_parse_rmc>,
  &parseTyped<minmea_sentence_ths, SentenceTHS, minmea_parse_ths>,
  &parseTyped<minmea_sentence_vtg, SentenceVTG, minmea_parse_vtg>,
  &parseTyped<minmea_sentence_zda, SentenceZDA, minmea_parse_zda>,
};

}  // namespace

namespace libgnss::nmea
{

NMEAReader::NMEAReader()
  : visitor_(std::make_unique<SentenceVisitor>(*this))
{
}

struct NMEAReader::SentenceVisitor
{
  explicit SentenceVisitor(NMEAReader& reader)
    : reader_(reader)
  {
  }

  void operator()(const SentenceGBS& gbs) const
  {
    // satellite fault detection, used to support Receiver Autonomous Integrity Monitoring (RAIM)
    store(gbs);
  }

  void operator()(const SentenceGGA& gga) const
  {
    // most common message for position, fix status and altitude
    store(gga);

    reader_.latest_fix_.status = gga.status;
    reader_.latest_fix_.latitude = gga.latitude;
    reader_.latest_fix_.longitude = gga.longitude;
    reader_.latest_fix_.altitude = gga.altitude;
    approximateCovariance(gga.hdop);
    getTimestamp(gga.time);
  }

  void operator()(const SentenceGLL& gll) const
  {
    // geographic position, latitude and longitude
    store(gll);

    if (!gll.valid)
    {
      return;
    }

    reader_.latest_fix_.status = statusFromFAA(gll.mode);
    reader_.latest_fix_.latitude = gll.latitude;
    reader_.latest_fix_.longitude = gll.longitude;
    getTimestamp(gll.time);
  }

  void operator()(const SentenceGSA& gsa) const
  {
    // dilution of precision and satellite status, used to approximate position covariance
    store(gsa);

    if (gsa.fix_type == GSAFixType::FIX_2D)
    {
      approximateCovariance(gsa.hdop);
    }
    else if (gsa.fix_type == GSAFixType::FIX_3D)
    {
      approximateCovariance(gsa.hdop, gsa.vdop);
      reader_.known_vdop_ = true;
    }
  }

  void operator()(const SentenceGST& gst) const
  {
    // pseudorange noise statistics, used to calculate precise position covariance
    store(gst);

    reader_.latest_fix_.position_covariance[0] =
      gst.longitude_error_deviation * gst.longitude_error_deviation;  // East
    reader_.latest_fix_.position_covariance[4] =
      gst.latitude_error_deviation * gst.latitude_error_deviation;    // North
    reader_.latest_fix_.position_covariance[8] =
      gst.altitude_error_deviation * gst.altitude_error_deviation;    // Up
    reader_.latest_fix_.covariance_type = Fix::CovarianceType::KNOWN;
    getTimestamp(gst.time);
  }

  void operator()(const SentenceGSV& gsv) const
  {
    // satellites in view, used for satellite status monitoring
    store(gsv);
  }

  void operator()(const SentenceRMC& rmc) const
  {
    // recommended minimum specific GNSS data, used for position, speed and course over ground
    store(rmc);

    if (!rmc.valid)
    {
      return;
    }

    reader_.latest_fix_.status = statusFromFAA(rmc.mode);
    reader_.latest_fix_.latitude = rmc.latitude;
    reader_.latest_fix_.longitude = rmc.longitude;
    reader_.latest_fix_.speed = rmc.speed * 1.852;  // convert from knots to kph
    reader_.latest_fix_.course = rmc.course;
    reader_.latest_fix_.variation = rmc.variation;
    reader_.latest_date_ = rmc.date;
    getTimestamp(rmc.time);
  }

  void operator()(const SentenceTHS& ths) const
  {
    // true heading and status
    store(ths);

    if (ths.mode != Mode::NO_FIX && ths.mode != Mode::NOT_VALID)
    {
      reader_.latest_fix_.heading = ths.heading;
    }
  }

  void operator()(const SentenceVTG& vtg) const
  {
    // course over ground and ground speed
    store(vtg);

    if (vtg.mode != Mode::NO_FIX && vtg.mode != Mode::NOT_VALID)
    {
      reader_.latest_fix_.course = vtg.coarse_true;
      reader_.latest_fix_.speed = vtg.speed_kph;
    }
  }

  void operator()(const SentenceZDA& zda) const
  {
    // time and date
    store(zda);

    reader_.latest_date_ = zda.date;
    getTimestamp(zda.time);
  }

private:
  template <typename T>
  void store(const T& sentence) const
  {
    std::get<std::optional<T>>(reader_.sentences).emplace(sentence);
  }

  void approximateCovariance(const float hdop, const float vdop = 0) const
  {
    if (
      reader_.latest_fix_.covariance_type == Fix::CovarianceType::UNKNOWN ||
      reader_.latest_fix_.covariance_type == Fix::CovarianceType::APPROXIMATED)
    {
      return;
    }

    auto approximateAccuracy = [](const FixStatus status) -> float
    {
      // all measurements in metres
      switch (status)
      {
        case FixStatus::GPS_FIX:
          return 5.0;
        case FixStatus::DGPS_FIX:
          return 2.0;
        case FixStatus::RTK_FIX:
          return 0.02;
        case FixStatus::RTK_FLOAT:
          return 0.5;
        default:
          return std::numeric_limits<float>::quiet_NaN();
      }
    };

    if (const float acc = approximateAccuracy(reader_.latest_fix_.status); !std::isnan(acc))
    {
      const double h_stddev = hdop * acc;
      const double h_variance = h_stddev * h_stddev;
      // assumes variance is split equally between east and north
      reader_.latest_fix_.position_covariance[0] = h_variance / 2;  // East
      reader_.latest_fix_.position_covariance[4] = h_variance / 2;  // North
      if (vdop > 0)
      {
        // vertical uncertainty is usually worse than horizontal
        const double v_stddev = 3 * h_stddev;
        const double v_variance = v_stddev * v_stddev;
        reader_.latest_fix_.position_covariance[8] = v_variance;  // Up
      }
      reader_.latest_fix_.covariance_type = Fix::CovarianceType::APPROXIMATED;
    }
  }

  void getTimestamp(const Time& time) const
  {
    if (!reader_.latest_date_.has_value())
    {
      return;
    }

    const minmea_date m_date = {
      reader_.latest_date_->day,
      reader_.latest_date_->month,
      reader_.latest_date_->year,
    };
    const minmea_time m_time = {
      time.hours,
      time.minutes,
      time.seconds,
      static_cast<int>(time.microseconds),
    };

    timespec ts;
    if (minmea_gettime(&ts, &m_date, &m_time) == 0)
    {
      reader_.latest_fix_.timestamp = system_clock::time_point{
        duration_cast<system_clock::duration>(seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec})};
    }
  }

  NMEAReader& reader_;
};

std::optional<Sentence> NMEAReader::parseNMEA(const char* sentence) const
{
  const size_t id = minmea_sentence_id(sentence, false);

  if (id <= MINMEA_UNKNOWN || id >= std::size(dispatch_table))
  {
    return std::nullopt;
  }

  std::optional<Sentence> s = dispatch_table[id](sentence);
  if (s.has_value())
  {
    std::visit(*visitor_, s.value());
  }

  return s;
}

Fix NMEAReader::getLatestFix() const
{
  return latest_fix_;
}

template <typename T>
void NMEAReader::setCustomCallback(utils::Callback<T> callback)
{
  std::get<utils::Callback<T>>(custom_callbacks_).emplace(std::move(callback));
}

}  // namespace libgnss::nmea