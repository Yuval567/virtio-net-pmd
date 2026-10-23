#include "data_structures/mbuf_mempool.h"
#include "utils.h"

#include "spdlog/spdlog.h"

MbufMempool::MbufMempool(const dma_memory_region& buffer_region, uint32_t num_mbufs, uint16_t buffer_size)
    : buffer_region_(buffer_region), mbufs_(std::make_unique<mbuf[]>(num_mbufs)), free_mbufs_(static_cast<size_t>(num_mbufs) * 2)
{
    for (uint32_t i = 0; i < num_mbufs; ++i)
    {
        mbuf& buf = mbufs_[i];
        buf.buf_addr = static_cast<uint8_t*>(buffer_region_.address) + static_cast<uint64_t>(i) * buffer_size;
        buf.buf_iova = buffer_region_.iova + static_cast<uint64_t>(i) * buffer_size;
        buf.buf_len = buffer_size;
        buf.pool = this;
        buf.reset();

        free_mbufs_.enqueue_elem(&buf);
    }
}

mbuf* MbufMempool::allocate() noexcept
{
    mbuf* buf;
    if (!free_mbufs_.dequeue_elem(buf))
    {
        return nullptr;
    }

    return buf;
}

void MbufMempool::free(mbuf* buf)
{
    buf->reset();

    if (!free_mbufs_.enqueue_elem(buf))
    {
        LOG_AND_THROW("Mbuf mempool free-list overflow (likely a double-free)");
    }
}

uint32_t MbufMempool::allocate_bulk(mbuf** out, uint32_t count) noexcept
{
    return static_cast<uint32_t>(free_mbufs_.dequeue_burst(out, count));
}

void MbufMempool::free_bulk(mbuf* const* bufs, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        bufs[i]->reset();
    }

    if (!free_mbufs_.enqueue_bulk(bufs, count))
    {
        LOG_AND_THROW("Mbuf mempool free-list overflow (likely a double-free)");
    }
}
