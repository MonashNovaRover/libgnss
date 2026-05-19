//
// Created by Terry on 5/17/2026.
//

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <reflex/matcher.h>

class PacketStreamMatcher final : public reflex::Matcher
{
public:
  PacketStreamMatcher(const std::string& regex_pattern, std::vector<std::string> packets)
    : reflex::Matcher(regex_pattern)
    , packets_(std::move(packets))
  {
  }

protected:
  size_t get(char* out, const size_t capacity) override
  {
    if (capacity == 0)
    {
      return 0;
    }

    size_t written = 0;
    while (written < capacity && packet_index_ < packets_.size())
    {
      const std::string& packet = packets_[packet_index_];
      const size_t remaining = packet.size() - packet_offset_;

      if (remaining == 0)
      {
        ++packet_index_;
        packet_offset_ = 0;
        continue;
      }

      const size_t chunk = std::min(capacity - written, remaining);
      std::memcpy(out + written, packet.data() + packet_offset_, chunk);
      written += chunk;
      packet_offset_ += chunk;

      if (packet_offset_ == packet.size())
      {
        ++packet_index_;
        packet_offset_ = 0;
      }
    }

    return written;
  }

private:
  std::vector<std::string> packets_;
  size_t packet_index_ = 0;
  size_t packet_offset_ = 0;
};

int main()
{
  std::vector<std::string> packets = {
    "asd", "hello ", "world qwe", " another hello w", "orld segment."};

  PacketStreamMatcher matcher("(?i)hello world", packets);

  // Keep the matcher in incremental mode so it refills its own sliding buffer
  // through get() instead of buffering the whole stream up front.
  matcher.buffer(8);

  std::cout << "--- RE/flex Incremental Buffering Example ---\n";
  std::cout << "Packets are provided as fragmented input, but one matcher pulls"
            << " them incrementally through get().\n\n";

  while (matcher.find() != 0)
  {
    std::cout << "[MATCH FOUND]: \"" << matcher.text() << "\""
              << " at absolute position: " << matcher.first() << "\n";
  }

  return 0;
}
