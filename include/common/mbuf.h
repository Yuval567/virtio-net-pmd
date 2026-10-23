#pragma once

#include <cstdint>

class MbufMempool;

/**
 * @brief Packet buffer descriptor.
 *
 * Points into a pool-owned DMA region. The payload lives at buf_addr + data_off.
 */
struct mbuf
{
    /** @brief Virtual address of the buffer. */
    uint8_t* buf_addr;

    /** @brief IOVA of the buffer, as the device sees it. */
    uint64_t buf_iova;

    /** @brief Size of the buffer in bytes. */
    uint16_t buf_len;

    /** @brief Offset of the payload within the buffer. */
    uint16_t data_off;

    /** @brief Length of the payload in bytes. */
    uint32_t data_len;

    /** @brief Pointer to the pool that owns this mbuf. */
    MbufMempool* pool;

    /** @brief Virtual address of the payload. */
    uint8_t* data() noexcept { return buf_addr + data_off; }
    const uint8_t* data() const noexcept { return buf_addr + data_off; }

    /** @brief IOVA of the payload, as the device sees it. */
    uint64_t data_iova() const noexcept { return buf_iova + data_off; }

    /** @brief Clears the payload offset and length. */
    void reset() noexcept { data_off = 0; data_len = 0; }
};
