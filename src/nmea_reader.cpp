#include "nmea_reader.hpp"

#include <algorithm>
#include <cstdio>

#include "minmea.h"

namespace nova::libgnss {

namespace {

std::string nmeaTimeToString(const minmea_time& t) {
	char out[16] = {0};
	std::snprintf(out, sizeof(out), "%02d:%02d:%02d", t.hours, t.minutes, t.seconds);
	return std::string(out);
}

} // namespace

bool NmeaReader::processLine(const std::string& line, GnssFix& fix) {
	if (line.empty()) {
		return false;
	}

	if (!minmea_check(line.c_str(), true)) {
		return false;
	}

	switch (minmea_sentence_id(line.c_str(), false)) {
		case MINMEA_SENTENCE_GGA: {
			minmea_sentence_gga frame{};
			if (!minmea_parse_gga(&frame, line.c_str())) {
				return false;
			}
			fix.latitude = minmea_tocoord(&frame.latitude);
			fix.longitude = minmea_tocoord(&frame.longitude);
			fix.altitude_m = minmea_tofloat(&frame.altitude);
			fix.satellites = frame.satellites_tracked;
			fix.fix_quality = frame.fix_quality;
			fix.has_fix = frame.fix_quality > 0;
			fix.utc_time = nmeaTimeToString(frame.time);
			return true;
		}
		case MINMEA_SENTENCE_RMC: {
			minmea_sentence_rmc frame{};
			if (!minmea_parse_rmc(&frame, line.c_str())) {
				return false;
			}
			fix.latitude = minmea_tocoord(&frame.latitude);
			fix.longitude = minmea_tocoord(&frame.longitude);
			fix.speed_knots = minmea_tofloat(&frame.speed);
			fix.course_degrees = minmea_tofloat(&frame.course);
			fix.has_fix = (frame.valid == 'A');
			fix.utc_time = nmeaTimeToString(frame.time);
			return true;
		}
		case MINMEA_SENTENCE_GSA: {
			minmea_sentence_gsa frame{};
			if (!minmea_parse_gsa(&frame, line.c_str())) {
				return false;
			}
			fix.has_fix = frame.fix_type >= 2;
			return true;
		}
		default:
			return false;
	}
}

} // namespace nova::libgnss
