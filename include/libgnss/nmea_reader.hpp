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
 * @brief NMEA sentence parsing.
 * @authors Terry Tian
 */

#ifndef LIBGNSS_NMEA_READER_HPP
#define LIBGNSS_NMEA_READER_HPP

#include <optional>
#include <variant>
#include <memory>
#include <functional>
#include <mutex>

#include "nmea_types.hpp"
#include "variant_helper.hpp"

namespace libgnss::nmea
{

class NMEAReader
{
public:
  NMEAReader();

  // disable copying
  NMEAReader(const NMEAReader&) = delete;
  NMEAReader& operator=(const NMEAReader&) = delete;

  /**
   * Parses an NMEA sentence and returns a variant containing the appropriate struct.
   * @param sentence NMEA sentence to parse, must be null-terminated
   * @return std::optional containing the parsed NMEASentence if successful, or std::nullopt if
   * parsing failed
   */
  std::optional<Sentence> parseNMEA(const char* sentence);

  /**
   * Returns the latest fix information, which is updated automatically with parsed NMEA sentences.
   * @return latest fix information
   */
  [[nodiscard]] Fix getLatestFix() const;

  /**
   * Returns the latest parsed sentence of a specific type, if available.
   * @tparam T NMEA sentence struct type, e.g. SentenceGGA
   * @return std::optional containing the latest parsed sentence of type T, or std::nullopt if no
   * such sentence has been parsed yet
   */
  template <typename T>
  std::optional<T> getLatestSentence() const
  {
    std::scoped_lock lock(mutex_);
    return std::get<std::optional<T>>(sentences);
  }

  /**
   * Registers a custom callback for a specific NMEA sentence type. The callback will be called
   * with the parsed sentence struct whenever a sentence of that type is successfully parsed.
   * @tparam T NMEA sentence struct type, e.g. SentenceGGA
   * @param callback callback function to register, takes a const reference to the parsed sentence
   * struct as its argument
   */
  template <typename T>
  void setCustomCallback(utils::Callback<T> callback)
  {
    std::scoped_lock lock(mutex_);
    std::get<utils::Callback<T>>(custom_callbacks_) = std::move(callback);
  }

  /**
   * Resets the NMEA reader, clearing all state data and stored sentences. Custom callbacks will not
   * be cleared.
   */
  void reset();

private:
  struct SentenceVisitor;
  std::unique_ptr<SentenceVisitor> visitor_;

  utils::variant_to_tuple_opts<Sentence> sentences;
  utils::variant_to_tuple_cbs<Sentence> custom_callbacks_;

  mutable std::mutex mutex_;

  Fix latest_fix_;
  std::optional<Date> latest_date_;
  bool known_vdop_ = false;
};

}  // namespace libgnss::nmea

#endif  // LIBGNSS_NMEA_READER_HPP