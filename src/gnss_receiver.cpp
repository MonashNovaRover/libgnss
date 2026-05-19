//
// Created by Terry on 5/7/2026.
//

#include <utility>

#include "libgnss/gnss_receiver.hpp"

namespace libgnss
{

GNSSReceiver::GNSSReceiver(std::string port_name, const uint16_t baud_rate)
  : port_name_(std::move(port_name))
  , baud_rate_(baud_rate)
  , running_(false)
{
}

void GNSSReceiver::start()
{
  if (running_.exchange(true))
  {
    return;
  }

  serial_port_.open(port_name_, baud_rate_);
  read_thread_ = std::thread(&GNSSReceiver::readLoop, this);
}

void GNSSReceiver::stop()
{
  if (!running_.exchange(false))
  {
    return;
  }

  if (read_thread_.joinable())
  {
    read_thread_.join();
  }
  serial_port_.close();
}

void GNSSReceiver::configurePort(const std::string& port_name, const int baud_rate)
{
  port_name_ = port_name;
  baud_rate_ = baud_rate;
}

int GNSSReceiver::writeBytes(const std::vector<uint8_t>& data, const unsigned int timeout_ms) const
{
  return serial_port_.write(data.data(), data.size(), timeout_ms);
}

int GNSSReceiver::writeString(const std::string& data, const unsigned int timeout_ms) const
{
  return serial_port_.write(
    reinterpret_cast<const uint8_t*>(data.c_str()), data.size(), timeout_ms);
}

nmea::Fix GNSSReceiver::latestFix() const
{
  return nmea_reader_.latestFix();
}

void GNSSReceiver::setRTCMCallback(utils::Callback<std::vector<uint8_t>> callback)
{
  rtcm_callback_ = std::move(callback);
}

void GNSSReceiver::unsetRTCMCallback()
{
  rtcm_callback_ = nullptr;
}

void GNSSReceiver::readLoop()
{
  std::array<uint8_t, SERIAL_RX_BUFFER_SIZE> buffer{};

  while (running_.load())
  {
    const int n = serial_port_.read(buffer.data(), buffer.size(), 1000);
    if (n > 0)
    {
      read_buffer_raw_.writeBuff(buffer.data(), n);
    }
  }
}

}  // namespace libgnss