#pragma once

#include <cstdint>
#include <memory>

#include "data_structures/dma_memory_region.h"
#include "data_structures/mbuf.h"
#include "data_structures/ring_buffer.h"

class MbufMempool
{
public:
    MbufMempool(const dma_memory_region& buffer_region, uint32_t num_mbufs, uint16_t buffer_size);

    MbufMempool(const MbufMempool&) = delete;
    MbufMempool& operator=(const MbufMempool&) = delete;

    mbuf* allocate() noexcept;
    void free(mbuf* buf);

    uint32_t allocate_bulk(mbuf** out, uint32_t count) noexcept;
    void free_bulk(mbuf* const* bufs, uint32_t count);

private:
    dma_memory_region buffer_region_;
    std::unique_ptr<mbuf[]> mbufs_;

    RingBuffer<mbuf*> free_mbufs_;
};
