#pragma once

#include <cstdint>

#include "data_structures/dma_memory_region.h"
#include "data_structures/virtqueue_layout.h"
#include "vfio_handler.h"

// One split virtqueue: the descriptor table plus the available/used rings, allocated as
// DMA memory and sized to whatever the device reports for that queue. Also owns a fixed pool
// of per-descriptor DMA packet buffers (one buffer per descriptor index, no chaining) so
// callers never need to manage DMA memory themselves - they just pick a slot, fill/read its
// buffer, and publish/consume it.
class Virtqueue
{
public:
    Virtqueue(VfioHandle& vfio_handle, uint16_t queue_size);
    ~Virtqueue();

    Virtqueue(const Virtqueue&) = delete;
    Virtqueue& operator=(const Virtqueue&) = delete;

    uint16_t size() const noexcept { return queue_size_; }
    uint64_t desc_iova() const noexcept { return desc_region_.iova; }
    uint64_t avail_iova() const noexcept { return avail_region_.iova; }
    uint64_t used_iova() const noexcept { return used_region_.iova; }

    // The per-queue offset (into the notify capability's BAR region) to write to in order to
    // kick this queue. Only meaningful once set by VirtioDevice::setup_virtqueue.
    void set_notify_off(uint16_t notify_off) noexcept { notify_off_ = notify_off; }
    uint16_t notify_off() const noexcept { return notify_off_; }

    // The DMA buffer backing descriptor `slot` (0 <= slot < size()).
    void* buffer_address(uint16_t slot) const;
    uint64_t buffer_iova(uint16_t slot) const;

    // Fills in descriptor `slot` to point at its own buffer and appends it to the avail
    // ring, in the order the spec requires (descriptor fully written before it becomes
    // visible to the device via avail.idx). `flags` is VIRTQ_DESC_F_WRITE for a
    // device-writable (RX) buffer, or 0 for a device-readable (TX) buffer.
    void publish(uint16_t slot, uint32_t len, uint16_t flags);

    // True if the device has completed at least one buffer we haven't consumed yet.
    bool has_completed() const;

    // Consumes and returns the next completed entry from the used ring. Only call when
    // has_completed() is true.
    virtq_used_elem take_completed();

private:
    volatile virtq_desc* desc_table() const;
    volatile virtq_avail_header* avail_header() const;
    volatile uint16_t* avail_ring() const;
    volatile virtq_used_header* used_header() const;
    volatile virtq_used_elem* used_ring() const;

    VfioHandle& vfio_handle_;
    uint16_t queue_size_;
    uint16_t notify_off_ = 0;
    uint16_t last_used_idx_ = 0;

    dma_memory_region desc_region_;
    dma_memory_region avail_region_;
    dma_memory_region used_region_;
    dma_memory_region buffer_region_;
};
