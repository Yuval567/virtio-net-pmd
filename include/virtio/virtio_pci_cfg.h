#pragma once

#include <cstdint>

/**
 * @brief Virtio PCI common configuration structure.
 *
 * This structure represents the common configuration registers for a Virtio device
 * that is accessed through PCI. It contains fields for device features, driver features,
 * queue configuration, and device status.
 */
struct virtio_pci_common_cfg
{
    // About the whole device
    uint32_t device_feature_select; // read-write
    uint32_t device_feature;        // read-only for driver
    uint32_t driver_feature_select; // read-write
    uint32_t driver_feature;        // read-write
    uint16_t config_msix_vector;    // read-write
    uint16_t num_queues;            // read-only for driver
    uint8_t device_status;          // read-write
    uint8_t config_generation;      // read-only for driver

    // About a specific virtqueue
    uint16_t queue_select;      // read-write
    uint16_t queue_size;        // read-write
    uint16_t queue_msix_vector; // read-write
    uint16_t queue_enable;      // read-write
    uint16_t queue_notify_off;  // read-only for driver
    uint64_t queue_desc;        // read-write
    uint64_t queue_driver;      // read-write (a.k.a. queue_avail pre-1.1)
    uint64_t queue_device;      // read-write (a.k.a. queue_used pre-1.1)

    // Present only if VIRTIO_F_NOTIF_CONFIG_DATA / VIRTIO_F_RING_RESET negotiated
    uint16_t queue_notify_data; // read-only for driver
    uint16_t queue_reset;       // read-write

    // About the administration virtqueue - present only if VIRTIO_F_ADMIN_VQ negotiated
    uint16_t admin_queue_index; // read-only for driver
    uint16_t admin_queue_num;   // read-only for driver
};
