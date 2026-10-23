#pragma once

#include <cstdint>
#include <memory>

#include "vfio/vfio_handler.h"
#include "common/dma_memory_region.h"
#include "common/mbuf.h"
#include "common/ring_buffer.h"
#include "virtio/virtqueue_layout.h"

/**
 * @brief Represents a virtqueue, the data structure used to exchange buffers with a virtio device.
 *
 * Owns the DMA memory for the descriptor table, avail ring and used ring, and
 * tracks which mbuf was posted in which descriptor slot.
 */
class Virtqueue
{
public:
    /**
     * @param vfio_handle VFIO handle, used to allocate the DMA memory for the rings.
     * @param queue_index Index of the queue.
     * @param queue_size Number of descriptors in the queue, must be a power of 2.
     * @param notify_addr Doorbell address, written with queue_index to kick the device.
     */
    Virtqueue(VfioHandle& vfio_handle, uint16_t queue_index, uint16_t queue_size, volatile uint16_t* notify_addr);

    /** @brief Frees the ring memory. */
    ~Virtqueue();

    Virtqueue(const Virtqueue&) = delete;
    Virtqueue& operator=(const Virtqueue&) = delete;


    /** @brief Ring accessors */
    inline volatile virtq_desc* desc_table() const noexcept { return static_cast<volatile virtq_desc*>(desc_region_.address); }
    inline volatile virtq_avail_header* avail_header() const noexcept { return static_cast<volatile virtq_avail_header*>(avail_region_.address); }
    inline volatile uint16_t* avail_ring() const noexcept { return reinterpret_cast<volatile uint16_t*>(static_cast<volatile uint8_t*>(avail_region_.address) + sizeof(virtq_avail_header)); }
    inline volatile virtq_used_header* used_header() const noexcept { return static_cast<volatile virtq_used_header*>(used_region_.address); }
    inline volatile virtq_used_elem* used_ring() const noexcept { return reinterpret_cast<volatile virtq_used_elem*>(static_cast<volatile uint8_t*>(used_region_.address) + sizeof(virtq_used_header)); }

    inline uint64_t desc_iova() const noexcept { return desc_region_.iova; }
    inline uint64_t avail_iova() const noexcept { return avail_region_.iova; }
    inline uint64_t used_iova() const noexcept { return used_region_.iova; }

    /** @brief Rings the doorbell. */
    inline void notify() const noexcept { *notify_addr_ = queue_index; }

    /**
     * @brief Posts mbufs to the device and notifies it.
     * @param bufs The mbufs to post.
     * @param num_buffers Number of mbufs to post.
     * @param flags Flags to set in the descriptor, e.g. VIRTQ_DESC_F_WRITE.
     * @return Number posted, limited by free descriptor slots. The caller keeps the rest.
     */
    uint32_t publish_burst(mbuf** bufs, uint16_t num_buffers, uint16_t flags);

    /**
     * @brief Collects mbufs the device has finished with, with data_len set from the used length.
     * @param out Receives the mbufs.
     * @param max_count Maximum number of mbufs to collect.
     * @return Number collected.
     */
    uint32_t pop_used_burst(mbuf** out, uint32_t max_count) noexcept;

    const uint16_t queue_size;
    const uint16_t queue_index;

private:
    VfioHandle& vfio_handle_;

    volatile uint16_t* notify_addr_;
    uint16_t last_used_idx_ = 0; ///< How far we have consumed the used ring.
    RingBuffer<uint16_t> free_slots_;

    std::unique_ptr<uint16_t[]> slot_scratch_;
    std::unique_ptr<mbuf* []> slot_ring_;

    dma_memory_region desc_region_;
    dma_memory_region avail_region_;
    dma_memory_region used_region_;
};
