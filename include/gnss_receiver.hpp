#ifndef LIBGNSS__GNSS_RECEIVER_HPP
#define LIBGNSS__GNSS_RECEIVER_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "nmea_reader.hpp"
#include "ringbuffer.hpp"
#include "rtcm_handler.hpp"
#include "serial.hpp"

#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE 8192 // 8KB
#endif

namespace nova::libgnss {

struct ReceiverConfig {
	std::string device;
	int baud_rate = 115200;
	int read_timeout_ms = 100;
};

class GnssReceiver {
public:
	using RtcmCallback = std::function<void(const std::vector<uint8_t>&)>;

	GnssReceiver();
	~GnssReceiver();

	GnssReceiver(const GnssReceiver&) = delete;
	GnssReceiver& operator=(const GnssReceiver&) = delete;

	bool start(const ReceiverConfig& config);
	void stop();
	bool isRunning() const;

	bool latestFix(GnssFix& out_fix) const;
	void setRtcmCallback(RtcmCallback callback);
	const std::string& lastError() const;

private:
	enum class StreamMode {
		Idle,
		Nmea,
		Rtcm,
	};

	void readLoop();
	void ingestBytes(const uint8_t* data, size_t len);
	void processIngress();
	void processByte(uint8_t byte);
	void finalizeNmeaSentence();
	void finalizeRtcmFrame();

	SerialPort serial_;
	NmeaReader nmea_reader_;
	RtcmHandler rtcm_handler_;
	jnk0le::Ringbuffer<uint8_t, 16384, false, 64> ingress_buffer_;

	StreamMode stream_mode_;
	std::string nmea_sentence_buffer_;
	bool drop_current_nmea_;
	std::vector<uint8_t> rtcm_frame_buffer_;
	size_t rtcm_expected_len_;

	std::atomic<bool> running_;
	std::thread read_thread_;

	mutable std::mutex fix_mutex_;
	GnssFix latest_fix_;
	bool has_latest_fix_;

	mutable std::mutex callback_mutex_;
	RtcmCallback rtcm_callback_;

	std::string last_error_;
};

} // namespace nova::libgnss

#endif // LIBGNSS__GNSS_RECEIVER_HPP
