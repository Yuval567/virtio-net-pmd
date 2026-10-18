#pragma once

#include <cstdint>

#include "vfio_handler.h"
#include "data_structures/dma_memory_region.h"
#include "data_structures/virtqueue_layout.h"

class Virtqueue
{
public:
    Virtqueue(VfioHandle& vfio_handle, uint16_t queue_index, uint16_t queue_size, volatile uint16_t* notify_addr);
    ~Virtqueue();

    Virtqueue(const Virtqueue&) = delete;
    Virtqueue& operator=(const Virtqueue&) = delete;

    inline volatile virtq_desc* desc_table() const noexcept { return static_cast<volatile virtq_desc*>(desc_region_.address); }
    inline volatile virtq_avail_header* avail_header() const noexcept { return static_cast<volatile virtq_avail_header*>(avail_region_.address); }
    inline volatile uint16_t* avail_ring() const noexcept { return reinterpret_cast<volatile uint16_t*>(static_cast<volatile uint8_t*>(avail_region_.address) + sizeof(virtq_avail_header)); }
    inline volatile virtq_used_header* used_header() const noexcept { return static_cast<volatile virtq_used_header*>(used_region_.address); }
    inline volatile virtq_used_elem* used_ring() const noexcept { return reinterpret_cast<volatile virtq_used_elem*>(static_cast<volatile uint8_t*>(used_region_.address) + sizeof(virtq_used_header)); }

    inline uint64_t desc_iova() const noexcept { return desc_region_.iova; }
    inline uint64_t avail_iova() const noexcept { return avail_region_.iova; }
    inline uint64_t used_iova() const noexcept { return used_region_.iova; }

    inline void notify() const noexcept;

    const uint16_t queue_size;
    const uint16_t queue_index;

private:
    VfioHandle& vfio_handle_;

    volatile uint16_t* notify_addr_;

    dma_memory_region desc_region_;
    dma_memory_region avail_region_;
    dma_memory_region used_region_;
};