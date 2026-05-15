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
 * @brief Functions for interacting with RTCM messages; currently only handles message validation.
 * @authors Terry Tian
 */

#ifndef LIBGNSS_RTCM_HANDLER_HPP
#define LIBGNSS_RTCM_HANDLER_HPP

#include <cstdint>

namespace libgnss
{

/**
 * Validates an RTCM message by checking the preamble, length, and CRC.
 * @param message Pointer to the start of the RTCM message
 * @param payload_len Length of the RTCM data payload in bytes (excluding the preamble, reserved,
 * length, and CRC fields)
 * @param buffer_size Total size of the buffer pointed to by message, in bytes.
 * @return true if the message is valid, false otherwise
 */
bool validateRTCM(const uint8_t* message, uint16_t payload_len, size_t buffer_size);

}  // namespace libgnss

#endif  // LIBGNSS_RTCM_HANDLER_HPP
