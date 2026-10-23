#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>

/**
 * @brief Lock-free Single-Producer Single-Consumer (SPSC) bounded ring buffer.
 *
 * One slot is always left empty to tell full from empty, so it holds at most
 * capacity - 1 items. The burst variants move as many items as fit, the bulk
 * variants move all of them or nothing.
 */
template <typename T>
class RingBuffer
{
public:
    /** 
     * @param capacity Number of slots, must be a non-zero power of 2. 
     * @throws std::invalid_argument if capacity is not a non-zero power of 2.
     */
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

    /** @return The number of slots. */
    size_t capacity() const noexcept
    {
        return capacity_;
    }

    /** 
     * @brief Adds one item. 
     * @param item The item to add.
     * @return false if full. 
     */
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

    /** 
     * @brief Adds up to count items. 
     * @param items The items to add.
     * @param count The number of items to add.
     * @return Number added. 
     */
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

    /** 
     * @brief Adds all count items or none. 
     * @param items The items to add.
     * @param count The number of items to add.
     * @return false if there is not enough room.
     */
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

    /** 
     * @brief Removes one item. 
     * @param item The item to remove.
     * @return false if empty. 
     */
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

    /** 
     * @brief Removes up to max_items items. 
     * @param output The items to remove.
     * @param max_items The maximum number of items to remove.
     * @return Number removed. 
     */
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

    /** 
     * @brief Removes exactly count items or none. 
     * @param output The items to remove.
     * @param count The number of items to remove.
     * @return false if fewer are available. 
     * */
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

private:
    const size_t capacity_;
    const size_t mask_;
    std::unique_ptr<T[]> buffer_;

    // Enforce alignment to 64 bytes to prevent False Sharing 
    // by ensuring head and tail reside on different cache lines.
    alignas(64) std::atomic_size_t head_;
    alignas(64) std::atomic_size_t tail_;
};
