#include <cstring>
#include <libserialport.h>

#include "serial.hpp"

namespace nova::libgnss
{

SerialPort::SerialPort()
  : port_(nullptr)
  , read_timeout_ms_(100)
{
}

SerialPort::~SerialPort()
{
  close();
}

bool SerialPort::open(const std::string& device, int baud_rate, int read_timeout_ms)
{
  close();
  read_timeout_ms_ = read_timeout_ms;

  sp_port* port = nullptr;
  sp_return ret = sp_get_port_by_name(device.c_str(), &port);
  if (ret != SP_OK || port == nullptr)
  {
    last_error_ = "sp_get_port_by_name failed for " + device;
    return false;
  }

  ret = sp_open(port, SP_MODE_READ_WRITE);
  if (ret != SP_OK)
  {
    last_error_ = "sp_open failed for " + device;
    sp_free_port(port);
    return false;
  }

  if (
    sp_set_baudrate(port, baud_rate) != SP_OK ||
    sp_set_bits(port, 8) != SP_OK ||
    sp_set_parity(port, SP_PARITY_NONE) != SP_OK ||
    sp_set_stopbits(port, 1) != SP_OK ||
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE) != SP_OK)
  {
    last_error_ = "sp_set_* serial configuration failed";
    sp_close(port);
    sp_free_port(port);
    return false;
  }

  port_ = port;
  last_error_.clear();
  return true;
}

void SerialPort::close()
{
  if (port_ != nullptr)
  {
    sp_port* port = static_cast<sp_port*>(port_);
    sp_close(port);
    sp_free_port(port);
    port_ = nullptr;
  }
}

bool SerialPort::isOpen() const
{
  return port_ != nullptr;
}

int SerialPort::read(uint8_t* buffer, size_t size)
{
  if (buffer == nullptr || size == 0)
  {
    return 0;
  }

  if (port_ == nullptr)
  {
    last_error_ = "serial read called on closed port";
    return -1;
  }

  sp_port* port = static_cast<sp_port*>(port_);
  int rc = sp_blocking_read(port, buffer, size, static_cast<unsigned int>(read_timeout_ms_));
  if (rc < 0)
  {
    last_error_ = "sp_blocking_read failed";
    return -1;
  }
  return rc;
}

int SerialPort::write(const uint8_t* buffer, size_t size)
{
  if (buffer == nullptr || size == 0)
  {
    return 0;
  }

  if (port_ == nullptr)
  {
    last_error_ = "serial write called on closed port";
    return -1;
  }

  sp_port* port = static_cast<sp_port*>(port_);
  int rc = sp_blocking_write(port, buffer, size, static_cast<unsigned int>(read_timeout_ms_));
  if (rc < 0)
  {
    last_error_ = "sp_blocking_write failed";
    return -1;
  }
  return rc;
}

const std::string& SerialPort::lastError() const
{
  return last_error_;
}

}  // namespace nova::libgnss
