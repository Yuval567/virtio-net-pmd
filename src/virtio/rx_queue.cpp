#include "virtio/rx_queue.h"
#include "common/utils.h"

#include <algorithm>

#include "spdlog/spdlog.h"

RxQueue::RxQueue(std::unique_ptr<Virtqueue> virtqueue, MbufMempool& mempool)
    : virtqueue_(std::move(virtqueue)), mempool_(mempool),
    refill_mbufs_scratch_(std::make_unique<mbuf* []>(virtqueue_->queue_size))
{
    populate_buffers();
}

void RxQueue::populate_buffers()
{
    const uint16_t queue_size = virtqueue_->queue_size;
    const uint32_t allocated = mempool_.allocate_bulk(refill_mbufs_scratch_.get(), queue_size);
    if (allocated != queue_size)
    {
        LOG_AND_THROW("Mbuf pool exhausted while populating RX queue (pool too small for queue_size)");
    }

    const uint32_t published = virtqueue_->publish_burst(refill_mbufs_scratch_.get(), static_cast<uint16_t>(allocated), VIRTQ_DESC_F_WRITE);
    if (published != queue_size)
    {
        LOG_AND_THROW("Failed to publish all initial RX buffers (queue_size larger than the virtqueue allows?)");
    }

    spdlog::info("Populated RX queue with {} buffers", queue_size);
}

void RxQueue::refill(uint32_t count)
{
    pending_refill_ += count;
    if (pending_refill_ == 0)
    {
        return;
    }

    const uint32_t request = std::min(pending_refill_, static_cast<uint32_t>(virtqueue_->queue_size));
    const uint32_t allocated = mempool_.allocate_bulk(refill_mbufs_scratch_.get(), request);
    if (allocated == 0)
    {
        return; // pool still exhausted; retry on a later call
    }

    const uint32_t published = virtqueue_->publish_burst(refill_mbufs_scratch_.get(), static_cast<uint16_t>(allocated), VIRTQ_DESC_F_WRITE);
    if (published < allocated)
    {
        // Some buffers couldn't be published, return them to the pool for retry later
        mempool_.free_bulk(refill_mbufs_scratch_.get() + published, allocated - published);
    }

    pending_refill_ -= published;
}

uint32_t RxQueue::receive_burst(mbuf** out_mbufs, uint32_t max_packets)
{
    const uint32_t request = std::min<uint32_t>(max_packets, virtqueue_->queue_size);

    // used ring is capped at queue_size per call
    const uint32_t num_completed = virtqueue_->pop_used_burst(out_mbufs, request);

    uint32_t num_received = 0;
    for (uint32_t i = 0; i < num_completed; ++i)
    {
        mbuf* buf = out_mbufs[i];

        if (buf->data_len < sizeof(virtio_net_hdr))
        {
            spdlog::warn("Received truncated buffer at iova=0x{:x} (len={} < sizeof(virtio_net_hdr)={}), dropping",
                buf->data_iova(), buf->data_len, sizeof(virtio_net_hdr));
            mempool_.free(buf);
            continue;
        }

        buf->data_off += sizeof(virtio_net_hdr);
        buf->data_len -= sizeof(virtio_net_hdr);
        out_mbufs[num_received++] = buf; // compact in place; safe since num_received <= i always
    }

    // every consumed descriptor - delivered or dropped - needs a replacement
    refill(num_completed);
    return num_received;
}