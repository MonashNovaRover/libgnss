//
// Created by Terry on 5/9/2026.
//

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

class SerialPort
{
public:
  SerialPort();
  ~SerialPort();

  // disable copying
  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;

  void open(const std::string& port_name, int baud_rate);
  void close();

  [[nodiscard]] bool isOpen() const;

  int read(uint8_t* buffer, size_t count, unsigned int timeout_ms) const;
  int write(const uint8_t* buffer, size_t count, unsigned int timeout_ms) const;

  class SerialPortError : public std::runtime_error
  {
  public:
    explicit SerialPortError(const std::string& msg)
      : runtime_error(msg)
    {
    }
  };

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
