//
// Created by Terry on 5/7/2026.
//

#ifndef LIBGNSS_GNSS_RECEIVER_HPP
#define LIBGNSS_GNSS_RECEIVER_HPP

#include <array>
#include <functional>
#include <string>

#include "libgnss/serial.hpp"
#include "libgnss/nmea_reader.hpp"

namespace libgnss
{

class GNSSReceiver
{
public:
  GNSSReceiver();
  ~GNSSReceiver();

  void start();
  void stop();
  void write(const std::string& data);
  void configurePort(const std::string& port_name, int baud_rate);

  template <typename T>
  void setSentenceCallback(utils::Callback<T> callback);

  // disable copying
  GNSSReceiver(const GNSSReceiver&) = delete;
  GNSSReceiver& operator=(const GNSSReceiver&) = delete;

private:
  SerialPort serial_port_;
  nmea::NMEAReader nmea_reader_;

  std::string port_name_;
  int baud_rate_;
};

}  // namespace libgnss

#endif  // LIBGNSS_GNSS_RECEIVER_HPP
