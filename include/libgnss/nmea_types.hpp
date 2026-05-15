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

/**
 * @brief NMEA sentence types. Mostly a C++ wrapper around the minmea structs.
 * @authors Terry Tian
 */

#ifndef LIBGNSS_NMEA_TYPES_HPP
#define LIBGNSS_NMEA_TYPES_HPP

#include <string>
#include <limits>
#include <chrono>
#include <array>

#include "minmea.h"

namespace libgnss::nmea
{

enum class FixStatus : uint8_t
{
  NO_FIX,
  NOT_VALID,
  GPS_FIX,
  DGPS_FIX,
  PPS_FIX,
  RTK_FIX,
  RTK_FLOAT,
  ESTIMATED,
  MANUAL,
  SIMULATED,
  PRECISE,
};

enum class Mode : char
{
  NO_FIX = 'N',
  NOT_VALID = 'V',
  AUTONOMOUS = 'A',
  DIFFERENTIAL = 'D',
  PRECISE = 'P',
  RTK_FIX = 'R',
  RTK_FLOAT = 'F',
  ESTIMATED = 'E',
  MANUAL = 'M',
  SIMULATED = 'S',
};

constexpr FixStatus statusFromFAA(const Mode mode)
{
  switch (mode)
  {
    case Mode::NO_FIX:
      return FixStatus::NO_FIX;
    case Mode::NOT_VALID:
      return FixStatus::NOT_VALID;
    case Mode::AUTONOMOUS:
      return FixStatus::GPS_FIX;
    case Mode::DIFFERENTIAL:
      return FixStatus::DGPS_FIX;
    case Mode::PRECISE:
      return FixStatus::PRECISE;
    case Mode::RTK_FIX:
      return FixStatus::RTK_FIX;
    case Mode::RTK_FLOAT:
      return FixStatus::RTK_FLOAT;
    case Mode::ESTIMATED:
      return FixStatus::ESTIMATED;
    case Mode::MANUAL:
      return FixStatus::MANUAL;
    case Mode::SIMULATED:
      return FixStatus::SIMULATED;
    default:
      return FixStatus::NOT_VALID;
  }
}

struct Fix
{
  enum class CovarianceType : uint8_t
  {
    UNKNOWN,
    APPROXIMATED,
    DIAGONAL_KNOWN,
    KNOWN,
  };

  std::chrono::system_clock::time_point timestamp;
  FixStatus status = FixStatus::NO_FIX;
  double latitude = std::numeric_limits<double>::quiet_NaN();
  double longitude = std::numeric_limits<double>::quiet_NaN();
  double altitude = std::numeric_limits<double>::quiet_NaN();
  double speed = std::numeric_limits<double>::quiet_NaN();
  double course = std::numeric_limits<double>::quiet_NaN();
  double heading = std::numeric_limits<double>::quiet_NaN();
  double variation = std::numeric_limits<double>::quiet_NaN();
  std::array<double, 9> position_covariance{};  // East, North and Up (ENU) in row-major order
  CovarianceType covariance_type = CovarianceType::UNKNOWN;
};

struct Date
{
  uint8_t day;
  uint8_t month;
  uint16_t year;

  explicit Date(const minmea_date& date)
    : day(date.day)
    , month(date.month)
    , year(date.year)
  {
  }
};

struct Time
{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint32_t microseconds;

  explicit Time(const minmea_time& time)
    : hours(time.hours)
    , minutes(time.minutes)
    , seconds(time.seconds)
    , microseconds(time.microseconds)
  {
  }
};

struct ID
{
  std::string talker;
  std::string sentence;
  std::string full;

  explicit ID(const minmea_type& type)
    : talker(type.talker_id, 2)
    , sentence(type.sentence_id, 3)
    , full(type.buf, 5)
  {
  }
};

struct SentenceGBS
{
  ID id;
  Time time;
  float err_latitude;
  float err_longitude;
  float err_altitude;
  int svid;
  float prob;
  float bias;
  float stddev;

  explicit SentenceGBS(const minmea_sentence_gbs& sentence)
    : id(sentence.type)
    , time(sentence.time)
    , err_latitude(minmea_tofloat(&sentence.err_latitude))
    , err_longitude(minmea_tofloat(&sentence.err_longitude))
    , err_altitude(minmea_tofloat(&sentence.err_altitude))
    , svid(sentence.svid)
    , prob(minmea_tofloat(&sentence.prob))
    , bias(minmea_tofloat(&sentence.bias))
    , stddev(minmea_tofloat(&sentence.stddev))
  {
  }
};

struct SentenceGGA
{
  ID id;
  Time time;
  float latitude;
  float longitude;
  FixStatus status;
  unsigned int sats;
  float hdop;
  float altitude;
  char altitude_units;
  float height;
  char height_units;
  float dgps_age;

  explicit SentenceGGA(const minmea_sentence_gga& sentence)
    : id(sentence.type)
    , time(sentence.time)
    , latitude(minmea_tofloat(&sentence.latitude))
    , longitude(minmea_tofloat(&sentence.longitude))
    , status(static_cast<FixStatus>(sentence.fix_quality))
    , sats(sentence.satellites_tracked)
    , hdop(minmea_tofloat(&sentence.hdop))
    , altitude(minmea_tofloat(&sentence.altitude))
    , altitude_units(sentence.altitude_units)
    , height(minmea_tofloat(&sentence.height))
    , height_units(sentence.height_units)
    , dgps_age(minmea_tofloat(&sentence.dgps_age))
  {
  }
};

struct SentenceGLL
{
  ID id;
  float latitude;
  float longitude;
  Time time;
  bool valid;
  Mode mode;

  explicit SentenceGLL(const minmea_sentence_gll& sentence)
    : id(sentence.type)
    , latitude(minmea_tofloat(&sentence.latitude))
    , longitude(minmea_tofloat(&sentence.longitude))
    , time(sentence.time)
    , valid(sentence.status != 'V')
    , mode(static_cast<Mode>(sentence.mode))
  {
  }
};

enum class GSAMode : char
{
  AUTO = 'A',
  FORCED = 'M',
};

enum class GSAFixType : uint8_t
{
  NONE = 1,
  FIX_2D = 2,
  FIX_3D = 3,
};

struct SentenceGSA
{
  ID id;
  GSAMode mode;
  GSAFixType fix_type;
  std::array<int, 12> sats{};
  float pdop;
  float hdop;
  float vdop;

  explicit SentenceGSA(const minmea_sentence_gsa& sentence)
    : id(sentence.type)
    , mode(static_cast<GSAMode>(sentence.mode))
    , fix_type(static_cast<GSAFixType>(sentence.fix_type))
    , pdop(minmea_tofloat(&sentence.pdop))
    , hdop(minmea_tofloat(&sentence.hdop))
    , vdop(minmea_tofloat(&sentence.vdop))
  {
    std::copy(std::begin(sentence.sats), std::end(sentence.sats), sats.begin());
  }
};

struct SentenceGST
{
  ID id;
  Time time;
  float rms_deviation;
  float semi_major_deviation;
  float semi_minor_deviation;
  float semi_major_orientation;
  float latitude_error_deviation;
  float longitude_error_deviation;
  float altitude_error_deviation;

  explicit SentenceGST(const minmea_sentence_gst& sentence)
    : id(sentence.type)
    , time(sentence.time)
    , rms_deviation(minmea_tofloat(&sentence.rms_deviation))
    , semi_major_deviation(minmea_tofloat(&sentence.semi_major_deviation))
    , semi_minor_deviation(minmea_tofloat(&sentence.semi_minor_deviation))
    , semi_major_orientation(minmea_tofloat(&sentence.semi_major_orientation))
    , latitude_error_deviation(minmea_tofloat(&sentence.latitude_error_deviation))
    , longitude_error_deviation(minmea_tofloat(&sentence.longitude_error_deviation))
    , altitude_error_deviation(minmea_tofloat(&sentence.altitude_error_deviation))
  {
  }
};

struct SatelliteInfo
{
  int nr;
  int elevation;
  int azimuth;
  float snr;

  SatelliteInfo() = default;

  explicit SatelliteInfo(const minmea_sat_info& info)
    : nr(info.nr)
    , elevation(info.elevation)
    , azimuth(info.azimuth)
    , snr(minmea_tofloat(&info.snr))
  {
  }
};

struct SentenceGSV
{
  ID id;
  int total_msgs;
  int msg_nr;
  int total_sats;
  std::array<SatelliteInfo, 4> sats{};

  explicit SentenceGSV(const minmea_sentence_gsv& sentence)
    : id(sentence.type)
    , total_msgs(sentence.total_msgs)
    , msg_nr(sentence.msg_nr)
    , total_sats(sentence.total_sats)
  {
    for (size_t i = 0; i < sats.size(); ++i)
    {
      sats[i] = SatelliteInfo(sentence.sats[i]);
    }
  }
};

struct SentenceRMC
{
  ID id;
  Time time;
  bool valid;
  float latitude;
  float longitude;
  float speed;
  float course;
  Date date;
  float variation;
  Mode mode;

  explicit SentenceRMC(const minmea_sentence_rmc& sentence)
    : id(sentence.type)
    , time(sentence.time)
    , valid(sentence.valid)
    , latitude(minmea_tofloat(&sentence.latitude))
    , longitude(minmea_tofloat(&sentence.longitude))
    , speed(minmea_tofloat(&sentence.speed))
    , course(minmea_tofloat(&sentence.course))
    , date(sentence.date)
    , variation(minmea_tofloat(&sentence.variation))
    , mode(static_cast<Mode>(sentence.mode))
  {
  }
};

struct SentenceTHS
{
  ID id;
  float heading;
  Mode mode;

  explicit SentenceTHS(const minmea_sentence_ths& sentence)
    : id(sentence.type)
    , heading(minmea_tofloat(&sentence.heading))
    , mode(static_cast<Mode>(sentence.mode))
  {
  }
};

struct SentenceVTG
{
  ID id;
  float coarse_true;
  float coarse_magnetic;
  float speed_knots;
  float speed_kph;
  Mode mode;

  explicit SentenceVTG(const minmea_sentence_vtg& sentence)
    : id(sentence.type)
    , coarse_true(minmea_tofloat(&sentence.true_track_degrees))
    , coarse_magnetic(minmea_tofloat(&sentence.magnetic_track_degrees))
    , speed_knots(minmea_tofloat(&sentence.speed_knots))
    , speed_kph(minmea_tofloat(&sentence.speed_kph))
    , mode(static_cast<Mode>(sentence.mode))
  {
  }
};

struct SentenceZDA
{
  ID id;
  Time time;
  Date date;
  int hour_offset;
  int minute_offset;

  explicit SentenceZDA(const minmea_sentence_zda& sentence)
    : id(sentence.type)
    , time(sentence.time)
    , date(sentence.date)
    , hour_offset(sentence.hour_offset)
    , minute_offset(sentence.minute_offset)
  {
  }
};

using Sentence = std::variant<
  SentenceGBS, SentenceGGA, SentenceGLL, SentenceGSA, SentenceGST, SentenceGSV, SentenceRMC,
  SentenceTHS, SentenceVTG, SentenceZDA>;

}  // namespace libgnss::nmea

#endif  // LIBGNSS_NMEA_TYPES_HPP
