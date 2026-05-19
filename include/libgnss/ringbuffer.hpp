//
// Created by Terry on 5/19/2026.
//

#ifndef LIBGNSS_BYTE_RINGBUFFER_HPP
#define LIBGNSS_BYTE_RINGBUFFER_HPP

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace nmea::utils
{

template <typename T, size_t Capacity>
class Ringbuffer
{
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
  static_assert(
    std::is_trivially_copyable_v<T>, "Overwriting SPSC ringbuffer requires trivially copyable T");

  struct Slot
  {
    std::atomic<size_t> sequence{0};
    std::atomic<T> value{};
  };

public:
  Ringbuffer()
    : write_sequence_(0)
    , read_sequence_(0)
  {
  }

  size_t count()
  {
    const size_t published = write_sequence_.load(std::memory_order_acquire);
    sync_read_sequence(published);
    return published - read_sequence_;
  }

  void push(T value)
  {
    const size_t current_write = write_sequence_.load(std::memory_order_relaxed);
    Slot& slot = slots_[current_write & buffer_mask_];

    slot.sequence.store(in_progress_sequence(current_write), std::memory_order_relaxed);
    slot.value.store(value, std::memory_order_relaxed);
    slot.sequence.store(committed_sequence(current_write), std::memory_order_release);
    write_sequence_.store(current_write + 1, std::memory_order_release);
  }

  size_t push(const T* values, size_t count)
  {
    if (values == nullptr || count == 0)
    {
      return 0;
    }

    size_t current_write = write_sequence_.load(std::memory_order_relaxed);

    for (size_t index = 0; index < count; ++index)
    {
      Slot& slot = slots_[current_write & buffer_mask_];

      slot.sequence.store(in_progress_sequence(current_write), std::memory_order_relaxed);
      slot.value.store(values[index], std::memory_order_relaxed);
      slot.sequence.store(committed_sequence(current_write), std::memory_order_release);
      ++current_write;
    }

    write_sequence_.store(current_write, std::memory_order_release);
    return count;
  }

  bool pop(T& value)
  {
    for (;;)
    {
      size_t published = write_sequence_.load(std::memory_order_acquire);
      if (sync_read_sequence(published) == 0)
      {
        return false;
      }

      const size_t current_read = read_sequence_;
      if (!try_read(current_read, value))
      {
        continue;
      }

      read_sequence_ = current_read + 1;
      return true;
    }
  }

  size_t pop(T* values, size_t count)
  {
    if (values == nullptr || count == 0)
    {
      return 0;
    }

    size_t popped = 0;

    while (popped < count)
    {
      if (!pop(values[popped]))
      {
        break;
      }

      ++popped;
    }

    return popped;
  }

  bool peek(T& value)
  {
    for (;;)
    {
      const size_t published = write_sequence_.load(std::memory_order_acquire);
      if (sync_read_sequence(published) == 0)
      {
        return false;
      }

      if (try_read(read_sequence_, value))
      {
        return true;
      }
    }
  }

  size_t peek(T* values, size_t count)
  {
    if (values == nullptr || count == 0)
    {
      return 0;
    }

    for (;;)
    {
      const size_t published = write_sequence_.load(std::memory_order_acquire);
      const size_t available = sync_read_sequence(published);
      if (available == 0)
      {
        return 0;
      }

      const size_t to_read = available < count ? available : count;
      size_t peeked = 0;

      while (peeked < to_read)
      {
        if (!try_read(read_sequence_ + peeked, values[peeked]))
        {
          break;
        }

        ++peeked;
      }

      if (peeked == to_read || peeked > 0)
      {
        return peeked;
      }
    }
  }

private:
  std::array<Slot, Capacity> slots_{};
  static constexpr size_t buffer_mask_ = Capacity - 1;
  // align indices to prevent hardware destructive interference (false sharing)
  alignas(CACHELINE_SIZE) std::atomic<size_t> write_sequence_;
  alignas(CACHELINE_SIZE) size_t read_sequence_;

  size_t sync_read_sequence(size_t published)
  {
    if (published - read_sequence_ > Capacity)
    {
      read_sequence_ = published - Capacity;
    }

    return published - read_sequence_;
  }

  bool try_read(size_t sequence, T& value)
  {
    Slot& slot = slots_[sequence & buffer_mask_];
    const size_t expected_sequence = committed_sequence(sequence);

    const size_t before = slot.sequence.load(std::memory_order_acquire);
    if (before != expected_sequence)
    {
      return false;
    }

    const T snapshot = slot.value.load(std::memory_order_relaxed);
    const size_t after = slot.sequence.load(std::memory_order_acquire);
    if (after != expected_sequence)
    {
      return false;
    }

    value = snapshot;
    return true;
  }

  static constexpr size_t in_progress_sequence(size_t sequence)
  {
    return (sequence << 1U) | 1U;
  }

  static constexpr size_t committed_sequence(size_t sequence)
  {
    return (sequence + 1U) << 1U;
  }
};

}  // namespace nmea::utils

#endif  // LIBGNSS_BYTE_RINGBUFFER_HPP
