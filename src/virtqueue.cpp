#include "virtqueue.h"
#include "utils.h"
#include "vfio_handler.h"
#include "data_structures/virtqueue_layout.h"

#include <algorithm>
#include <atomic>
#include <stdexcept>

#include "spdlog/spdlog.h"

Virtqueue::Virtqueue(VfioHandle& vfio_handle, uint16_t queue_index, uint16_t queue_size, volatile uint16_t* notify_addr)
    : queue_size(queue_size), queue_index(queue_index), vfio_handle_(vfio_handle), notify_addr_(notify_addr),
    free_slots_(static_cast<size_t>(queue_size) * 2), slot_scratch_(std::make_unique<uint16_t[]>(queue_size)),
    slot_ring_(std::make_unique<mbuf* []>(queue_size))
{
    const uint64_t desc_size = static_cast<uint64_t>(queue_size) * sizeof(virtq_desc);
    const uint64_t avail_size = sizeof(virtq_avail_header) + static_cast<uint64_t>(queue_size) * sizeof(uint16_t);
    const uint64_t used_size = sizeof(virtq_used_header) + static_cast<uint64_t>(queue_size) * sizeof(virtq_used_elem);

    desc_region_ = vfio_handle_.allocate_dma_memory(desc_size);
    avail_region_ = vfio_handle_.allocate_dma_memory(avail_size);
    used_region_ = vfio_handle_.allocate_dma_memory(used_size);

    for (uint16_t i = 0; i < queue_size; ++i)
    {
        free_slots_.enqueue_elem(i);
    }
}

Virtqueue::~Virtqueue()
{
    vfio_handle_.free_dma_memory(desc_region_);
    vfio_handle_.free_dma_memory(avail_region_);
    vfio_handle_.free_dma_memory(used_region_);
}

uint32_t Virtqueue::publish_burst(mbuf** bufs, uint16_t num_buffers, uint16_t flags)
{
    const uint32_t allocated = static_cast<uint32_t>(free_slots_.dequeue_burst(slot_scratch_.get(), num_buffers));
    if (allocated == 0)
    {
        return 0; // no free descriptor slots right now; caller handles the shortfall
    }

    volatile virtq_desc* desc = desc_table();
    volatile virtq_avail_header* avail_hdr = avail_header();
    volatile uint16_t* ring = avail_ring();
    const uint16_t base_avail_idx = avail_hdr->idx;

    for (uint32_t i = 0; i < allocated; ++i)
    {
        const uint16_t slot = slot_scratch_[i];
        desc[slot].addr = bufs[i]->data_iova();
        desc[slot].len = bufs[i]->buf_len;
        desc[slot].flags = flags;
        desc[slot].next = 0;
        slot_ring_[slot] = bufs[i];

        ring[static_cast<uint16_t>(base_avail_idx + i) % queue_size] = slot;
    }

    // Ensure all descriptors and avail ring entries are visible before updating avail->idx
    std::atomic_thread_fence(std::memory_order_release);
    avail_hdr->idx = static_cast<uint16_t>(base_avail_idx + allocated);

    // Ensure avail->idx update is visible before notifying the device
    std::atomic_thread_fence(std::memory_order_seq_cst);
    notify();

    return allocated;
}

uint32_t Virtqueue::pop_used_burst(mbuf** out, uint32_t max_count) noexcept
{
    const uint16_t current_used_idx = used_header()->idx;
    std::atomic_thread_fence(std::memory_order_acquire);

    const uint16_t available = static_cast<uint16_t>(current_used_idx - last_used_idx_);
    const uint32_t count = std::min({ max_count, static_cast<uint32_t>(available), static_cast<uint32_t>(queue_size) });

    volatile virtq_used_elem* ring = used_ring();

    for (uint32_t i = 0; i < count; ++i)
    {
        const volatile virtq_used_elem& used_elem = ring[static_cast<uint16_t>(last_used_idx_ + i) % queue_size];
        const uint16_t slot = static_cast<uint16_t>(used_elem.id);

        mbuf* buf = slot_ring_[slot];
        buf->data_len = used_elem.len;

        out[i] = buf;
        slot_scratch_[i] = slot;
    }

    // Always succeeds: these are exactly the slots we just took out of circulation, and
    // free_slots_ is sized to hold every slot at once.
    free_slots_.enqueue_bulk(slot_scratch_.get(), count);
    last_used_idx_ = static_cast<uint16_t>(last_used_idx_ + count);

    return count;
}
