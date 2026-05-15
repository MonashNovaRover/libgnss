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

#include "libgnss/serial.hpp"

#include <string>
#include <iostream>

namespace libgnss
{

SerialPort::SerialPort()
  : active_io_(0)
  , closing_(false)
  , port_(nullptr)
{
}

SerialPort::~SerialPort()
{
  try
  {
    close();
  }
  catch (const SerialPortError& e)
  {
    std::cerr << "Error closing serial port: " << e.what() << std::endl;
  }
}

void SerialPort::open(const std::string& port_name, const int baud_rate)
{
  close();
  sp_port* new_port = nullptr;
  check(sp_get_port_by_name(port_name.c_str(), &new_port));

  try
  {
  // configure port
    check(sp_open(new_port, SP_MODE_READ_WRITE));
    check(sp_set_baudrate(new_port, baud_rate));
    check(sp_set_bits(new_port, 8));
    check(sp_set_parity(new_port, SP_PARITY_NONE));
    check(sp_set_stopbits(new_port, 1));
    check(sp_set_flowcontrol(new_port, SP_FLOWCONTROL_NONE));
  }
  catch (...)
  {
    if (new_port != nullptr)
    {
      (void) sp_close(new_port);
      sp_free_port(new_port);
    }
    throw;
  }

  {
    const std::lock_guard lock(mutex_);
    port_ = new_port;
    closing_ = false;
  }
}

void SerialPort::close()
{
  sp_port* port_to_close = nullptr;

  {
    std::unique_lock lock(mutex_);
    if (port_ == nullptr)
    {
      closing_ = false;
      return;
    }

    closing_ = true;
    cv_.wait(lock, [this] { return active_io_ == 0; });

    port_to_close = port_;
    port_ = nullptr;
  }

  check(sp_close(port_to_close));
  sp_free_port(port_to_close);

  {
    const std::lock_guard lock(mutex_);
    closing_ = false;
  }
}

bool SerialPort::isOpen() const
{
  const std::lock_guard lock(mutex_);
  return port_ != nullptr;
}

int SerialPort::read(uint8_t* buffer, const size_t count, const unsigned int timeout_ms) const
{
  sp_port* const port = beginIO();
  try
  {
    const int result = check(sp_blocking_read_next(port, buffer, count, timeout_ms));
    endIO();
    return result;
  }
  catch (...)
  {
    endIO();
    throw;
  }
}

int SerialPort::write(
  const uint8_t* buffer, const size_t count, const unsigned int timeout_ms) const
{
  sp_port* const port = beginIO();
  try
  {
    const int result = check(sp_blocking_write(port, buffer, count, timeout_ms));
    endIO();
    return result;
  }
  catch (...)
  {
    endIO();
    throw;
  }
}

sp_port* SerialPort::beginIO() const
{
  const std::lock_guard lock(mutex_);
  if (port_ == nullptr || closing_)
  {
    throw SerialPortError("libserialport: port is not open");
  }

  ++active_io_;
  return port_;
}

void SerialPort::endIO() const noexcept
{
  const std::lock_guard lock(mutex_);
  if (active_io_ > 0)
  {
    --active_io_;
  }

  if (closing_ && active_io_ == 0)
  {
    cv_.notify_all();
  }
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