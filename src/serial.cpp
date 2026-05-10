//
// Created by Terry on 5/9/2026.
//

#include "serial.hpp"

#include <string>
#include <iostream>

namespace libgnss
{

SerialPort::SerialPort()
  : port_(nullptr)
{
}

SerialPort::~SerialPort()
{
  close();
}

void SerialPort::open(const std::string& port_name, const int baud_rate)
{
  close();
  check(sp_get_port_by_name(port_name.c_str(), &port_));
  // configure port
  check(sp_open(port_, SP_MODE_READ_WRITE));
  check(sp_set_baudrate(port_, baud_rate));
  check(sp_set_bits(port_, 8));
  check(sp_set_parity(port_, SP_PARITY_NONE));
  check(sp_set_stopbits(port_, 1));
  check(sp_set_flowcontrol(port_, SP_FLOWCONTROL_NONE));
}

void SerialPort::close()
{
  try
  {
    if (isOpen())
    {
      check(sp_close(port_));
      sp_free_port(port_);
      port_ = nullptr;
    }
  }
  catch (const SerialPortError& e)
  {
    std::cerr << "Error closing serial port: " << e.what() << std::endl;
  }
}

bool SerialPort::isOpen() const
{
  return port_ != nullptr;
}

int SerialPort::read(uint8_t* buffer, const size_t count) const
{
  return check(sp_blocking_read_next(port_, buffer, count, 0));
}

int SerialPort::write(
  const uint8_t* buffer, const size_t count, const unsigned int timeout_ms) const
{
  return check(sp_blocking_write(port_, buffer, count, timeout_ms));
}

int SerialPort::check(const sp_return result)
{
  switch (result)
  {
    case SP_ERR_ARG:
      throw SerialPortError("libserialport: invalid argument");
    case SP_ERR_FAIL:
    {
      char* error_message = sp_last_error_message();
      const std::string details = error_message != nullptr ? error_message : "unknown error";
      sp_free_error_message(error_message);
      throw SerialPortError("libserialport: operation failed: " + details);
    }
    case SP_ERR_SUPP:
      throw SerialPortError("libserialport: operation not supported");
    case SP_ERR_MEM:
      throw SerialPortError("libserialport: couldn't allocate memory");
    case SP_OK:
    default:
      return result;
  }
}

}  // namespace libgnss