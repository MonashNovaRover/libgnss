//
// Created by Terry on 5/7/2026.
//

#ifndef LIBGNSS_GNSS_RECEIVER_HPP
#define LIBGNSS_GNSS_RECEIVER_HPP

#include <array>
#include <functional>
#include <string>

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
  void configurePort(const std::string& port_name, int baud_rate);

  // disable copying
  GNSSReceiver(const GNSSReceiver&) = delete;
  GNSSReceiver& operator=(const GNSSReceiver&) = delete;

private:
  std::array<std::function<void(nmea::Sentence)>, std::variant_size_v<nmea::Sentence>>
    sentence_type_callback_map;

  std::string port_name_;
  int baud_rate_;
};

}  // namespace libgnss

#endif  // LIBGNSS_GNSS_RECEIVER_HPP
