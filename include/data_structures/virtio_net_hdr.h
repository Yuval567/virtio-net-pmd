#pragma once

#include <cstdint>

struct virtio_net_hdr
{
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;
};

static_assert(sizeof(virtio_net_hdr) == 12, "virtio_net_hdr must be 12 bytes for VirtIO 1.x");
