#ifndef LIBGNSS__SERIAL_HPP
#define LIBGNSS__SERIAL_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace nova::libgnss
{

class SerialPort
{
public:
  SerialPort();
  ~SerialPort();

  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;

  bool open(const std::string& device, int baud_rate, int read_timeout_ms);
  void close();
  bool isOpen() const;

  int read(uint8_t* buffer, size_t size);
  int write(const uint8_t* buffer, size_t size);

  const std::string& lastError() const;

private:
  void* port_;
  int read_timeout_ms_;
  std::string last_error_;
};

}  // namespace nova::libgnss

#endif // LIBGNSS__SERIAL_HPP
