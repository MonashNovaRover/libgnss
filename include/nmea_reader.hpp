#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nova::libgnss {

struct GnssFix {
	bool has_fix = false;
	double latitude = 0.0;
	double longitude = 0.0;
	double altitude_m = 0.0;
	double speed_knots = 0.0;
	double course_degrees = 0.0;
	int satellites = 0;
	int fix_quality = 0;
	std::string utc_time;
};

class NmeaReader {
public:
	bool processLine(const std::string& line, GnssFix& fix);
};

} // namespace nova::libgnss

