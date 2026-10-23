#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>

// Lock-free Single-Producer Single-Consumer (SPSC) bounded ring buffer.
template <typename T>
class RingBuffer
{
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1),
        buffer_(std::make_unique<T[]>(capacity)), head_(0), tail_(0)
    {
        if (capacity == 0 || (capacity & (capacity - 1)) != 0)
        {
            throw std::invalid_argument("RingBuffer capacity must be a non-zero power of 2");
        }
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    bool enqueue_elem(const T& item) noexcept
    {
        const size_t cur_head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (cur_head + 1) & mask_;

        if (next_head == tail_.load(std::memory_order_acquire))
        {
            return false; // Full
        }

        buffer_[cur_head] = item;

        head_.store(next_head, std::memory_order_release);

        return true;
    }

    size_t enqueue_burst(const T* items, size_t count) noexcept
    {
        if (count == 0) return 0;

        const size_t cur_head = head_.load(std::memory_order_relaxed);
        const size_t cur_tail = tail_.load(std::memory_order_acquire);

        const size_t free_slots = (cur_tail - cur_head - 1) & mask_; 
        const size_t enqueue_count = std::min(count, free_slots);

        for (size_t i = 0; i < enqueue_count; ++i)
        {
            buffer_[(cur_head + i) & mask_] = items[i];
        }

        head_.store((cur_head + enqueue_count) & mask_, std::memory_order_release);

        return enqueue_count;
    }

    bool enqueue_bulk(const T* items, size_t count) noexcept
    {
        const size_t cur_head = head_.load(std::memory_order_relaxed);
        const size_t cur_tail = tail_.load(std::memory_order_acquire);

        const size_t free_slots = (cur_tail - cur_head - 1) & mask_;
        if (count > free_slots)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            buffer_[(cur_head + i) & mask_] = items[i];
        }

        head_.store((cur_head + count) & mask_, std::memory_order_release);

        return true;
    }

    bool dequeue_elem(T& item) noexcept
    {
        const size_t cur_tail = tail_.load(std::memory_order_relaxed);

        if (cur_tail == head_.load(std::memory_order_acquire))
        {
            return false; // Empty
        }

        item = buffer_[cur_tail];

        tail_.store((cur_tail + 1) & mask_, std::memory_order_release);

        return true;
    }

    size_t dequeue_burst(T* output, size_t max_items) noexcept
    {
        if (max_items == 0) return 0;

        const size_t cur_tail = tail_.load(std::memory_order_relaxed);
        const size_t cur_head = head_.load(std::memory_order_acquire);

        const size_t available = (cur_head - cur_tail) & mask_;
        const size_t count = std::min(max_items, available);

        for (size_t i = 0; i < count; ++i)
        {
            output[i] = buffer_[(cur_tail + i) & mask_];
        }

        tail_.store((cur_tail + count) & mask_, std::memory_order_release);

        return count;
    }

    bool dequeue_bulk(T* output, size_t count) noexcept
    {
        const size_t cur_tail = tail_.load(std::memory_order_relaxed);
        const size_t cur_head = head_.load(std::memory_order_acquire);

        const size_t available = (cur_head - cur_tail) & mask_;
        if (count > available)
        {
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            output[i] = buffer_[(cur_tail + i) & mask_];
        }

        tail_.store((cur_tail + count) & mask_, std::memory_order_release);

        return true;
    }

    size_t capacity() const noexcept
    {
        return capacity_;
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::unique_ptr<T[]> buffer_;

    // Enforce alignment to 64 bytes to prevent False Sharing 
    // by ensuring head and tail reside on different cache lines.
    alignas(64) std::atomic_size_t head_;
    alignas(64) std::atomic_size_t tail_;
};
