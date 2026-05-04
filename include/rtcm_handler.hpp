#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nova::libgnss {

class RtcmHandler {
public:
	bool parseFrame(const std::vector<uint8_t>& frame) const;
};

} // namespace nova::libgnss

