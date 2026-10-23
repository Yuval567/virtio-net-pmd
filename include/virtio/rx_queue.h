#pragma once

#include <cstdint>
#include <memory>

#include "virtio/constants.h"
#include "virtio/virtqueue.h"
#include "common/mbuf.h"
#include "common/mbuf_mempool.h"
#include "virtio/virtio_net_hdr.h"

/** @brief Receives packets from a virtio-net device. */
class RxQueue
{
public:
    /**
     * @brief Takes ownership of the virtqueue and posts an initial full set of buffers.
     * @param mempool Source of RX buffers, must outlive this object.
     * @throws std::runtime_error if the pool is too small for the queue.
     */
    RxQueue(std::unique_ptr<Virtqueue> virtqueue, MbufMempool& mempool);

    RxQueue(const RxQueue&) = delete;
    RxQueue& operator=(const RxQueue&) = delete;

    /**
     * @brief Receives up to max_packets packets, with the virtio_net_hdr already stripped.
     *
     * The caller owns the returned mbufs and must free them back to the mempool.
     * Also refills the queue with fresh buffers.
     *
     * @param out_mbufs Receives the packets.
     * @return Number of packets received.
     */
    uint32_t receive_burst(mbuf** out_mbufs, uint32_t max_packets);

private:
    /** @brief Posts a full queue worth of buffers. Used once at startup. */
    void populate_buffers();

    /**
     * @brief Posts replacement buffers for count consumed ones, plus any earlier shortfall.
     * @param count Number of buffers to post.
     * @note If the pool or the queue can't take them all, the rest is retried on the next call.
     */
    void refill(uint32_t count);

    std::unique_ptr<Virtqueue> virtqueue_;
    MbufMempool& mempool_;
    uint32_t pending_refill_ = 0;

    std::unique_ptr<mbuf* []> refill_mbufs_scratch_;
};
