//
// Created by Terry on 5/19/2026.
//

#ifndef LIBGNSS_PARSERS_HPP
#define LIBGNSS_PARSERS_HPP

#include <cstdint>
#include <vector>
#include <optional>

namespace libgnss::parsers
{

class IParser
{
public:
  virtual ~IParser() = default;

  virtual void push_byte(uint8_t byte) = 0;
  virtual std::optional<std::vector<uint8_t>> try_match() = 0;
};

}  // namespace libgnss::parsers

#endif  // LIBGNSS_PARSERS_HPP
