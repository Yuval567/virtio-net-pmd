#pragma once

#include <cstdint>

/** @brief Virtio vendor ID. */
constexpr uint16_t VIRTIO_VENDOR_ID = 0x1af4;

/** @brief Virtio  device ID. */
constexpr uint16_t VIRTIO_NET_DEVICE_ID = 0x1041;

/** @brief Virtio PCI capability types. */
constexpr uint8_t VIRTIO_PCI_CAP_COMMON_CFG = 1;
constexpr uint8_t VIRTIO_PCI_CAP_NOTIFY_CFG = 2;
constexpr uint8_t VIRTIO_PCI_CAP_ISR_CFG = 3;
constexpr uint8_t VIRTIO_PCI_CAP_DEVICE_CFG = 4;
constexpr uint8_t VIRTIO_PCI_CAP_PCI_CFG = 5;
constexpr uint8_t VIRTIO_PCI_CAP_SHARED_MEMORY_CFG = 8;
constexpr uint8_t VIRTIO_PCI_CAP_VNDR_CFG = 9;

/** @brief Virtio status flags. */
constexpr uint8_t VIRTIO_STATUS_ACKNOWLEDGE = 1;
constexpr uint8_t VIRTIO_STATUS_DRIVER = 2;
constexpr uint8_t VIRTIO_STATUS_DRIVER_OK = 4;
constexpr uint8_t VIRTIO_STATUS_FEATURES_OK = 8;
constexpr uint8_t VIRTIO_STATUS_DEVICE_NEEDS_RESET = 64;
constexpr uint8_t VIRTIO_STATUS_FAILED = 128;

/** @brief Virtio feature bits. */
constexpr uint32_t VIRTIO_F_VERSION_1 = 32;
constexpr uint32_t VIRTIO_F_ACCESS_PLATFORM = 33;
constexpr uint64_t VIRTIO_F_VERSION_1_BIT = (1ULL << VIRTIO_F_VERSION_1);

/** 
 * @brief Supported features for this driver. 
 * @note This driver only supports Virtio 1.0 and the access platform feature.
*/
constexpr uint64_t VIRTIO_SUPPORTED_FEATURES = VIRTIO_F_VERSION_1_BIT | (1ULL << VIRTIO_F_ACCESS_PLATFORM);
