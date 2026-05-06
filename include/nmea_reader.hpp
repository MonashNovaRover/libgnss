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

#ifndef LIBGNSS__NMEA_READER_HPP
#define LIBGNSS__NMEA_READER_HPP

#include <string>
#include <optional>
#include <variant>

#include "minmea.h"

namespace libgnss
{

using NMEASentence = std::variant<
  minmea_sentence_gbs, minmea_sentence_gga, minmea_sentence_gll, minmea_sentence_gsa,
  minmea_sentence_gst, minmea_sentence_gsv, minmea_sentence_rmc, minmea_sentence_ths,
  minmea_sentence_vtg, minmea_sentence_zda>;

std::optional<NMEASentence> parseSentence(const std::string &sentence);

}  // namespace libgnss

#endif  // LIBGNSS__NMEA_READER_HPP