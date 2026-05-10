//
// Created by Terry on 5/9/2026.
//

#ifndef LIBGNSS_SERIAL_HPP
#define LIBGNSS_SERIAL_HPP

#include <cstdint>
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

  int read(uint8_t* buffer, size_t count) const;
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
  static int check(sp_return result);

  sp_port* port_;
};

}  // namespace libgnss

#endif  // LIBGNSS_SERIAL_HPP
