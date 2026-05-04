#include "rtcm_handler.hpp"

namespace nova::libgnss {

bool RtcmHandler::parseFrame(const std::vector<uint8_t>& frame) const {
	if (frame.size() < 6) {
		return false;
	}

	if (frame[0] != 0xD3) {
		return false;
	}

	const size_t payload_len = (static_cast<size_t>(frame[1] & 0x03) << 8) |
													 static_cast<size_t>(frame[2]);
	const size_t expected_frame_len = 3 + payload_len + 3;

	if (expected_frame_len > 4096) {
		return false;
	}

	return frame.size() == expected_frame_len;
}

} // namespace nova::libgnss

