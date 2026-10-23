#pragma once

#include <cstdint>
#include <memory>

#include "common/dma_memory_region.h"
#include "common/mbuf.h"
#include "common/ring_buffer.h"

/**
 * @brief Fixed pool of mbufs carved out of one DMA region.
 *
 * Free mbufs are kept in a ring.
 */
class MbufMempool
{
public:
    /**
     * @param buffer_region DMA region holding the packet buffers.
     * @param num_mbufs Number of mbufs to create.
     * @param buffer_size Size of each buffer in bytes.
     */
    MbufMempool(const dma_memory_region& buffer_region, uint32_t num_mbufs, uint16_t buffer_size);

    MbufMempool(const MbufMempool&) = delete;
    MbufMempool& operator=(const MbufMempool&) = delete;

    /** 
     * @brief Takes one mbuf from the pool.
     * @return The mbuf, or nullptr if the pool is empty.
     */
    mbuf* allocate() noexcept;

    /** 
     * @brief Returns an mbuf to the pool.
     * @param buf The mbuf to return.
     * @throws std::runtime_error on free-list overflow (double free).
     */
    void free(mbuf* buf);

    /**
     * @brief Takes up to count mbufs from the pool.
     * @param out Receives the mbufs.
     * @param count Maximum number of mbufs to allocate.
     * @return Number actually allocated, can be less than count.
     */
    uint32_t allocate_bulk(mbuf** out, uint32_t count) noexcept;

    /** 
     * @brief Returns count mbufs to the pool. 
     * @param bufs The mbufs to return.
     * @param count The number of mbufs to return.
     * @throws std::runtime_error on free-list overflow. 
     */
    void free_bulk(mbuf* const* bufs, uint32_t count);

private:
    dma_memory_region buffer_region_;
    std::unique_ptr<mbuf[]> mbufs_;

    RingBuffer<mbuf*> free_mbufs_;
};
