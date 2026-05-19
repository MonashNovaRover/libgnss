//
// Created by Terry on 5/7/2026.
//

#ifndef LIBGNSS_GNSS_RECEIVER_HPP
#define LIBGNSS_GNSS_RECEIVER_HPP

#ifndef SERIAL_RX_BUFFER_SIZE
#define SERIAL_RX_BUFFER_SIZE 4096
#endif

#include <string>
#include <thread>
#include <atomic>

#include "libgnss/serial.hpp"
#include "libgnss/nmea_reader.hpp"
#include "libgnss/parsers.hpp"
#include "ringbuffer/ringbuffer.hpp"

namespace libgnss
{

class GNSSReceiver
{
public:
  GNSSReceiver(std::string port_name, uint16_t baud_rate);

  // disable copying
  GNSSReceiver(const GNSSReceiver&) = delete;
  GNSSReceiver& operator=(const GNSSReceiver&) = delete;

  void start();
  void stop();
  void configurePort(const std::string& port_name, int baud_rate);
  int writeBytes(const std::vector<uint8_t>& data, unsigned int timeout_ms) const;
  int writeString(const std::string& data, unsigned int timeout_ms) const;
  nmea::Fix latestFix() const;

  template <typename TSentence>
  std::optional<TSentence> latestNMEASentence() const
  {
    return nmea_reader_.latestSentence<TSentence>();
  }

  // addParser
  // addOneTimeParser

  void setRTCMCallback(utils::Callback<std::vector<uint8_t>> callback);
  void unsetRTCMCallback();

  template <typename TSentence>
  void setNMEASentenceCallback(utils::Callback<TSentence> callback, const bool async = false)
  {
    nmea_reader_.setCustomCallback<TSentence>(std::move(callback), async);
  }

  template <typename TSentence>
  void unsetNMEASentenceCallback()
  {
    nmea_reader_.unsetCustomCallback<TSentence>();
  }

private:
  void readLoop();

  SerialPort serial_port_;
  nmea::NMEAReader nmea_reader_;
  jnk0le::Ringbuffer<uint8_t, SERIAL_RX_BUFFER_SIZE> read_buffer_;
  jnk0le::Ringbuffer<uint8_t, SERIAL_RX_BUFFER_SIZE> read_buffer_raw_;
  jnk0le::Ringbuffer<uint8_t, SERIAL_RX_BUFFER_SIZE / 2> read_buffer_unparsed_;

  std::string port_name_;
  int baud_rate_;
  std::thread read_thread_;
  std::atomic<bool> running_;
  std::vector<std::unique_ptr<parsers::IParser>> parsers_;
  utils::Callback<std::vector<uint8_t>> rtcm_callback_;
};

}  // namespace libgnss

#endif  // LIBGNSS_GNSS_RECEIVER_HPP
