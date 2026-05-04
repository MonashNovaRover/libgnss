#include "gnss_receiver.hpp"

#include <array>

namespace {

constexpr size_t kMaxNmeaSentenceLen = 1024;
constexpr size_t kMaxRtcmFrameLen = 4096;

} // namespace

namespace nova::libgnss {

GnssReceiver::GnssReceiver()
		: running_(false),
			has_latest_fix_(false),
			stream_mode_(StreamMode::Idle),
			drop_current_nmea_(false),
			rtcm_expected_len_(0) {}

GnssReceiver::~GnssReceiver() {
	stop();
}

bool GnssReceiver::start(const ReceiverConfig& config) {
	if (running_.load()) {
		return true;
	}

	if (!serial_.open(config.device, config.baud_rate, config.read_timeout_ms)) {
		last_error_ = serial_.lastError();
		return false;
	}

	running_.store(true);
	read_thread_ = std::thread(&GnssReceiver::readLoop, this);
	last_error_.clear();
	return true;
}

void GnssReceiver::stop() {
	const bool was_running = running_.exchange(false);
	if (!was_running) {
		return;
	}

	if (read_thread_.joinable()) {
		read_thread_.join();
	}
	serial_.close();
}

bool GnssReceiver::isRunning() const {
	return running_.load();
}

bool GnssReceiver::latestFix(GnssFix& out_fix) const {
	std::lock_guard<std::mutex> lock(fix_mutex_);
	if (!has_latest_fix_) {
		return false;
	}

	out_fix = latest_fix_;
	return true;
}

void GnssReceiver::setRtcmCallback(RtcmCallback callback) {
	std::lock_guard<std::mutex> lock(callback_mutex_);
	rtcm_callback_ = std::move(callback);
}

const std::string& GnssReceiver::lastError() const {
	return last_error_;
}

void GnssReceiver::readLoop() {
	std::array<uint8_t, 512> rx{};

	while (running_.load()) {
		const int n = serial_.read(rx.data(), rx.size());
		if (n < 0) {
			last_error_ = serial_.lastError();
			continue;
		}

		if (n == 0) {
			continue;
		}

		ingestBytes(rx.data(), static_cast<size_t>(n));
		processIngress();
	}
}

void GnssReceiver::ingestBytes(const uint8_t* data, size_t len) {
	if (data == nullptr || len == 0) {
		return;
	}

	for (size_t i = 0; i < len; ++i) {
		if (!ingress_buffer_.insert(data[i])) {
			// Drop the oldest byte when full to preserve the freshest stream data.
			ingress_buffer_.remove();
			ingress_buffer_.insert(data[i]);
		}
	}
}

void GnssReceiver::processIngress() {
	uint8_t byte = 0;
	while (ingress_buffer_.remove(&byte)) {
		processByte(byte);
	}
}

void GnssReceiver::processByte(uint8_t byte) {
	switch (stream_mode_) {
		case StreamMode::Idle:
			if (byte == '$') {
				nmea_sentence_buffer_.clear();
				nmea_sentence_buffer_.push_back(static_cast<char>(byte));
				drop_current_nmea_ = false;
				stream_mode_ = StreamMode::Nmea;
			} else if (byte == 0xD3) {
				rtcm_frame_buffer_.clear();
				rtcm_frame_buffer_.push_back(byte);
				rtcm_expected_len_ = 0;
				stream_mode_ = StreamMode::Rtcm;
			}
			break;

		case StreamMode::Nmea:
			if (byte == '$') {
				// Resync if a new sentence begins before newline termination.
				nmea_sentence_buffer_.clear();
				nmea_sentence_buffer_.push_back(static_cast<char>(byte));
				drop_current_nmea_ = false;
				break;
			}

			if (!drop_current_nmea_) {
				if (nmea_sentence_buffer_.size() < kMaxNmeaSentenceLen) {
					nmea_sentence_buffer_.push_back(static_cast<char>(byte));
				} else {
					nmea_sentence_buffer_.clear();
					drop_current_nmea_ = true;
				}
			}

			if (byte == '\n') {
				if (!drop_current_nmea_) {
					finalizeNmeaSentence();
				}
				nmea_sentence_buffer_.clear();
				drop_current_nmea_ = false;
				stream_mode_ = StreamMode::Idle;
			}
			break;

		case StreamMode::Rtcm:
			rtcm_frame_buffer_.push_back(byte);

			if (rtcm_frame_buffer_.size() == 3) {
				const size_t payload_len = (static_cast<size_t>(rtcm_frame_buffer_[1] & 0x03) << 8) |
														 static_cast<size_t>(rtcm_frame_buffer_[2]);
				rtcm_expected_len_ = 3 + payload_len + 3;
				if (rtcm_expected_len_ < 6 || rtcm_expected_len_ > kMaxRtcmFrameLen) {
					rtcm_frame_buffer_.clear();
					rtcm_expected_len_ = 0;
					stream_mode_ = StreamMode::Idle;
				}
			}

			if (rtcm_expected_len_ > 0 && rtcm_frame_buffer_.size() == rtcm_expected_len_) {
				finalizeRtcmFrame();
				rtcm_frame_buffer_.clear();
				rtcm_expected_len_ = 0;
				stream_mode_ = StreamMode::Idle;
			} else if (rtcm_frame_buffer_.size() > kMaxRtcmFrameLen) {
				rtcm_frame_buffer_.clear();
				rtcm_expected_len_ = 0;
				stream_mode_ = StreamMode::Idle;
			}
			break;
	}
}

void GnssReceiver::finalizeNmeaSentence() {
	while (!nmea_sentence_buffer_.empty() &&
				 (nmea_sentence_buffer_.back() == '\n' || nmea_sentence_buffer_.back() == '\r')) {
		nmea_sentence_buffer_.pop_back();
	}

	if (nmea_sentence_buffer_.empty()) {
		return;
	}

	GnssFix fix;
	if (!nmea_reader_.processLine(nmea_sentence_buffer_, fix)) {
		return;
	}

	std::lock_guard<std::mutex> lock(fix_mutex_);
	latest_fix_ = fix;
	has_latest_fix_ = true;
}

void GnssReceiver::finalizeRtcmFrame() {
	if (!rtcm_handler_.parseFrame(rtcm_frame_buffer_)) {
		return;
	}

	std::lock_guard<std::mutex> lock(callback_mutex_);
	if (rtcm_callback_) {
		rtcm_callback_(rtcm_frame_buffer_);
	}
}

} // namespace nova::libgnss

