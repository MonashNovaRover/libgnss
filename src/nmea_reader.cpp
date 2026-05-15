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
#include "nmea_reader.hpp"

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

std::optional<system_clock::time_point> getTimestamp(const Date& date, const Time& time)
{
  const minmea_date m_date = {
    date.day,
    date.month,
    date.year,
  };
  const minmea_time m_time = {
    time.hours,
    time.minutes,
    time.seconds,
    static_cast<int>(time.microseconds),
  };

  timespec ts;
  if (minmea_gettime(&ts, &m_date, &m_time) == -1)
  {
    return std::nullopt;
  }
  return system_clock::time_point{
    duration_cast<system_clock::duration>(seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec})};
}

void approximateCovariance(Fix& fix, const float hdop, const float vdop = 0)
{
  if (
    fix.covariance_type == Fix::CovarianceType::UNKNOWN ||
    fix.covariance_type == Fix::CovarianceType::APPROXIMATED)
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

  if (const float acc = approximateAccuracy(fix.status); !std::isnan(acc))
  {
    const double h_stddev = hdop * acc;
    const double h_variance = h_stddev * h_stddev;
    // assumes variance is split equally between east and north
    fix.position_covariance[0] = h_variance / 2;  // East
    fix.position_covariance[4] = h_variance / 2;  // North
    if (vdop > 0)
    {
      // vertical uncertainty is usually worse than horizontal
      const double v_stddev = 3 * h_stddev;
      const double v_variance = v_stddev * v_stddev;
      fix.position_covariance[8] = v_variance;  // Up
    }
    fix.covariance_type = Fix::CovarianceType::APPROXIMATED;
  }
}

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
    store(gbs);
    // used to support Receiver Autonomous Integrity Monitoring (RAIM)
    // TODO: NOT IMPLEMENTED
  }

  void operator()(const SentenceGGA& gga) const
  {
    store(gga);

    if (gga.status == FixStatus::NO_FIX)
    {
      reader_.latest_fix_.status = FixStatus::NO_FIX;
      return;
    }

    reader_.latest_fix_.status = gga.status;
    reader_.latest_fix_.latitude = gga.latitude;
    reader_.latest_fix_.longitude = gga.longitude;
    reader_.latest_fix_.altitude = gga.altitude;
    approximateCovariance(reader_.latest_fix_, gga.hdop);

    if (reader_.latest_date_.has_value())
    {
      if (
        const auto timestamp = getTimestamp(reader_.latest_date_.value(), gga.time);
        timestamp.has_value())
      {
        reader_.latest_fix_.timestamp = timestamp.value();
      }
    }
  }

  void operator()(const SentenceGLL&) const
  {
  }

  void operator()(const SentenceGSA&) const
  {
  }

  void operator()(const SentenceGST&) const
  {
  }

  void operator()(const SentenceGSV&) const
  {
  }

  void operator()(const SentenceRMC&) const
  {
  }

  void operator()(const SentenceTHS&) const
  {
  }

  void operator()(const SentenceVTG&) const
  {
  }

  void operator()(const SentenceZDA&) const
  {
  }

private:
  template <typename T>
  void store(const T& sentence) const
  {
    std::get<std::optional<T>>(reader_.sentences).emplace(sentence);
  }

  NMEAReader& reader_;
};

std::optional<Sentence> NMEAReader::parseNMEA(const char* sentence)
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