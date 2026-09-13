#pragma once

#include <cstdint>

// VIRTIO 1.x net spec, section 5.1.6.1: the header prefixed to every packet buffer.
// Always 12 bytes here since VIRTIO_F_VERSION_1 is negotiated - num_buffers is present
// regardless of whether VIRTIO_NET_F_MRG_RXBUF was negotiated; the driver just sets it to 0
// on transmit (and ignores it on receive) when that feature isn't in use, as it isn't here.
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

constexpr uint8_t VIRTIO_NET_HDR_GSO_NONE = 0;
