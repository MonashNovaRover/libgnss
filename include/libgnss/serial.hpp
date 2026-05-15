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
 * @brief libserialport wrapper.
 * @authors Terry Tian
 */

#ifndef LIBGNSS_SERIAL_HPP
#define LIBGNSS_SERIAL_HPP

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <string>
#include <libserialport.h>
#include <stdexcept>

namespace libgnss
{

/**
 * A wrapper class for libserialport that provides a simple interface for opening, closing, reading
 * from, and writing to serial ports. Access to the serial port is thread-safe.
 */
class SerialPort
{
public:
  /**
   * Exception type for errors related to serial port operations. This exception is thrown when an
   * error occurs while opening, closing, reading from, or writing to the serial port. The error
   * message will contain details about the specific error that occurred, which may include
   * information from libserialport if applicable.
   */
  class SerialPortError : public std::runtime_error
  {
  public:
    explicit SerialPortError(const std::string& msg)
      : runtime_error(msg)
    {
    }
  };

  SerialPort();
  ~SerialPort();

  // disable copying
  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;

  /**
   * Opens the specified serial port with the given baud rate and default settings (8N1, no flow
   * control). If the port is already open, it will be closed and reopened with the new settings.
   * @param port_name Name of the serial port to open (e.g., "/dev/ttyUSB0" on Linux, "COM3" on
   * Windows)
   * @param baud_rate Baud rate to use for the serial communication (e.g., 9600, 115200)
   * @throws SerialPortError if the port cannot be opened or configured
   */
  void open(const std::string& port_name, int baud_rate);

  /**
   * Closes the serial port. If there are any active read/write operations, this function will wait
   * for them to complete before closing the port.
   * @throws SerialPortError if an error occurs while closing the port
   */
  void close();

  /**
   * Checks if the serial port is currently open.
   * @return true if the port is open, false otherwise
   */
  [[nodiscard]] bool isOpen() const;

  /**
   * Reads up to count bytes from the serial port into the provided buffer, blocking until at least
   * one byte is available or the specified timeout has elapsed.
   * @param buffer Pointer to a buffer where the read data will be stored. The caller is responsible
   * for ensuring that the buffer is large enough to hold count bytes.
   * @param count Maximum number of bytes to read into the buffer.
   * @param timeout_ms Maximum time to wait for data to become available, in milliseconds. A value
   * of 0 means to wait indefinitely.
   * @return
   */
  int read(uint8_t* buffer, size_t count, unsigned int timeout_ms) const;

  /**
   * Writes up to count bytes from the provided buffer to the serial port, blocking until all bytes
   * have been written or the specified timeout has elapsed.
   * @param buffer Pointer to a buffer containing the data to be written. The caller is responsible
   * for ensuring that the buffer contains at least count bytes.
   * @param count Number of bytes to write from the buffer to the serial port.
   * @param timeout_ms Maximum time to wait for the write operation to complete, in milliseconds. A
   * value of 0 means to wait indefinitely.
   * @return
   */
  int write(const uint8_t* buffer, size_t count, unsigned int timeout_ms) const;

private:
  sp_port* beginIO() const;
  void endIO() const noexcept;
  static int check(sp_return result);

  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  mutable size_t active_io_;
  mutable bool closing_;
  sp_port* port_;
};

}  // namespace libgnss

#endif  // LIBGNSS_SERIAL_HPP
