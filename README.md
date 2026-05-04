# libgnss

C/C++ library for interfacing with GNSS modules.

## Current MVP Baseline

Implemented modules:
- `SerialPort` transport wrapper (`include/serial.hpp`, `src/serial.cpp`)
- `NmeaReader` sentence framing and parsing via `minmea` (`include/nmea_reader.hpp`, `src/nmea_reader.cpp`)
- `RtcmHandler` frame extraction passthrough (`include/rtcm_handler.hpp`, `src/rtcm_handler.cpp`)
- `GnssReceiver` orchestration thread (`include/gnss_receiver.hpp`, `src/gnss_receiver.cpp`)

Notes:
- NMEA parsing currently maps `GGA`, `RMC`, and `GSA` into a normalized `GnssFix`.
- RTCM handling currently performs frame extraction only (preamble + length based), no message-type decoding yet.
- `libserialport` support is optional at configure time. If not found, the build still succeeds but serial I/O returns runtime errors.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

To enable real serial I/O, install libserialport development files so CMake can find `serialport.h` and `libserialport`.
