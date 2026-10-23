#pragma once

#include <cstdint>

class MbufMempool;

struct mbuf
{
    uint8_t* buf_addr;
    uint64_t buf_iova;
    uint16_t buf_len;
    uint16_t data_off;
    uint32_t data_len;
    MbufMempool* pool;

    uint8_t* data() noexcept { return buf_addr + data_off; }
    const uint8_t* data() const noexcept { return buf_addr + data_off; }
    uint64_t data_iova() const noexcept { return buf_iova + data_off; }

    void reset() noexcept { data_off = 0; data_len = 0; }
};
