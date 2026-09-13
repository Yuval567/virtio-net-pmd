#pragma once

#include <cstdint>

#include "data_structures/dma_memory_region.h"
#include "vfio_handler.h"

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

    void set_notify_off(uint16_t notify_off) noexcept { notify_off_ = notify_off; }
    uint16_t notify_off() const noexcept { return notify_off_; }

private:
    VfioHandle& vfio_handle_;
    uint16_t queue_size_;
    uint16_t notify_off_ = 0;

    dma_memory_region desc_region_;
    dma_memory_region avail_region_;
    dma_memory_region used_region_;
};