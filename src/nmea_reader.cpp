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

#include <array>
#include <optional>
#include <string>

#include "minmea.h"
#include "nmea_reader.hpp"


namespace libgnss
{

using DispatchFn = std::optional<NMEASentence> (*)(const char*);

template <typename Frame, bool (*Fn)(Frame*, const char*)>
std::optional<NMEASentence> parseTyped(const char* s)
{
  Frame frame{};
  if (!Fn(&frame, s))
  {
    return std::nullopt;
  }
  return NMEASentence{frame};
}

// Index must match enum minmea_sentence_id values:
// 0 = UNKNOWN, 1 = GBS, 2 = GGA, ...
static constexpr DispatchFn dispatch_table[] = {
  nullptr,  // MINMEA_UNKNOWN
  &parseTyped<minmea_sentence_gbs, minmea_parse_gbs>,
  &parseTyped<minmea_sentence_gga, minmea_parse_gga>,
  &parseTyped<minmea_sentence_gll, minmea_parse_gll>,
  &parseTyped<minmea_sentence_gsa, minmea_parse_gsa>,
  &parseTyped<minmea_sentence_gst, minmea_parse_gst>,
  &parseTyped<minmea_sentence_gsv, minmea_parse_gsv>,
  &parseTyped<minmea_sentence_rmc, minmea_parse_rmc>,
  &parseTyped<minmea_sentence_ths, minmea_parse_ths>,
  &parseTyped<minmea_sentence_vtg, minmea_parse_vtg>,
  &parseTyped<minmea_sentence_zda, minmea_parse_zda>,
};

std::optional<NMEASentence> parseSentence(const std::string& sentence)
{
  const auto id = minmea_sentence_id(sentence.c_str(), false);

  if (id <= MINMEA_UNKNOWN || id >= static_cast<int>(std::size(dispatch_table)))
  {
    return std::nullopt;
  }

  auto fn = dispatch_table[static_cast<size_t>(id)];
  
  if (!fn)
  {
    return std::nullopt;
  }
  return fn(sentence.c_str());
}

}  // namespace libgnss