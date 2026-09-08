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