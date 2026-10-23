#pragma once

#include <cstdint>

struct virtq_desc
{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

constexpr uint16_t VIRTQ_DESC_F_NEXT = 1;     // buffer continues via `next`
constexpr uint16_t VIRTQ_DESC_F_WRITE = 2;    // buffer is device-write-only (otherwise device-read-only)
constexpr uint16_t VIRTQ_DESC_F_INDIRECT = 4; // buffer contains a list of descriptors

struct virtq_avail_header
{
    uint16_t flags;
    uint16_t idx;
};

struct virtq_used_elem
{
    uint32_t id;  // index of the descriptor chain head that was used
    uint32_t len; // total bytes the device wrote into that chain
};

struct virtq_used_header
{
    uint16_t flags;
    uint16_t idx;
};