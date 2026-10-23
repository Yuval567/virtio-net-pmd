#pragma once

#include <cstdint>
#include <memory>

#include "virtio/constants.h"
#include "virtio/virtqueue.h"
#include "common/mbuf.h"
#include "common/mbuf_mempool.h"
#include "virtio/virtio_net_hdr.h"

class RxQueue
{
public:
    RxQueue(std::unique_ptr<Virtqueue> virtqueue, MbufMempool& mempool);

    RxQueue(const RxQueue&) = delete;
    RxQueue& operator=(const RxQueue&) = delete;

    uint32_t receive_burst(mbuf** out_mbufs, uint32_t max_packets);

    void notify() const noexcept { virtqueue_->notify(); }

private:
    void populate_buffers();
    void refill(uint32_t count);

    std::unique_ptr<Virtqueue> virtqueue_;
    MbufMempool& mempool_;
    uint32_t pending_refill_ = 0;
    
    std::unique_ptr<mbuf* []> refill_mbufs_scratch_;
};
