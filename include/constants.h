#pragma once

#include <cstdint>

constexpr uint16_t VIRTIO_VENDOR_ID = 0x1af4;
constexpr uint16_t VIRTIO_NET_DEVICE_ID = 0x1041;

constexpr uint8_t VIRTIO_VENDOR_SPECIFIC_CAP_ID = 0x09;

constexpr uint8_t VIRTIO_PCI_CAP_COMMON_CFG = 1;
constexpr uint8_t VIRTIO_PCI_CAP_NOTIFY_CFG = 2;
constexpr uint8_t VIRTIO_PCI_CAP_ISR_CFG = 3;
constexpr uint8_t VIRTIO_PCI_CAP_DEVICE_CFG = 4;
constexpr uint8_t VIRTIO_PCI_CAP_PCI_CFG = 5;
constexpr uint8_t VIRTIO_PCI_CAP_SHARED_MEMORY_CFG = 8;
constexpr uint8_t VIRTIO_PCI_CAP_VNDR_CFG = 9;

constexpr uint8_t VIRTIO_STATUS_ACKNOWLEDGE = 1;
constexpr uint8_t VIRTIO_STATUS_DRIVER = 2;
constexpr uint8_t VIRTIO_STATUS_DRIVER_OK = 4;
constexpr uint8_t VIRTIO_STATUS_FEATURES_OK = 8;
constexpr uint8_t VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64;
constexpr uint8_t VIRTIO_STATUS_FAILED = 128;

constexpr uint32_t VIRTIO_F_VERSION_1 = 32;
constexpr uint32_t VIRTIO_F_ACCESS_PLATFORM = 33;

constexpr uint64_t VIRTIO_F_VERSION_1_BIT = (1ULL << VIRTIO_F_VERSION_1);

// Feature bits this driver knows how to drive and will accept if the device offers them.
// VIRTIO_F_VERSION_1 is mandatory (we only speak the modern, non-legacy protocol).
// VIRTIO_F_ACCESS_PLATFORM means: all addresses handed to the device (virtqueues, buffers)
// must be real IOVAs mapped via VFIO_IOMMU_MAP_DMA - which is how we allocate DMA memory
// anyway, so we always accept it.
constexpr uint64_t VIRTIO_SUPPORTED_FEATURES = VIRTIO_F_VERSION_1_BIT | (1ULL << VIRTIO_F_ACCESS_PLATFORM);

// VIRTIO 1.x net spec, section 5.1.2: without VIRTIO_NET_F_MQ, queue 0 is receiveq1 and
// queue 1 is transmitq1.
constexpr uint16_t VIRTIO_NET_RX_QUEUE_INDEX = 0;
constexpr uint16_t VIRTIO_NET_TX_QUEUE_INDEX = 1;

// Per-descriptor DMA buffer size: comfortably covers a max-size standard Ethernet frame
// (1518 bytes) plus the 12-byte virtio_net_hdr, with no need for descriptor chaining.
constexpr uint32_t VIRTQUEUE_BUFFER_SIZE = 2048;