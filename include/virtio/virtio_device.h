#pragma once

#include <memory>

#include "pcie/pci_device.h"
#include "vfio/vfio_handler.h"
#include "virtio/virtqueue.h"
#include "virtio/virtio_pci_cfg.h"

/**
 * @brief Modern (1.x) virtio-pci device.
 *
 * Locates the config structures, negotiates features and sets up virtqueues.
 */
class VirtioDevice
{
public:
    /**
     * @brief Checks the device is virtio-net, then resets it and negotiates features.
     * @param device_fd VFIO device fd.
     * @param pci_device The PCI device object, used to find the virtio capabilities.
     * @param vfio_handle The VFIO handle, used to interact with the VFIO device.
     * @throws std::runtime_error on a wrong device, missing capability or rejected features.
     */
    VirtioDevice(const int device_fd, const PCIDevice& pci_device, VfioHandle& vfio_handle);

    VirtioDevice(const VirtioDevice&) = delete;
    VirtioDevice& operator=(const VirtioDevice&) = delete;

    /**
     * @brief Sets up and enables the next queue in order (0 = rx, 1 = tx).
     * @throws std::runtime_error if the device doesn't have that queue.
     */
    std::unique_ptr<Virtqueue> create_virtqueue();

    /** @brief Sets DRIVER_OK. Create all queues before calling this. */
    void start();

private:
    /** @brief Parses the virtio vendor capabilities and records where each config structure is. */
    void initialize_capabilites();

    /** @brief Writes 0 to the status register and waits for the device to finish resetting. */
    void reset_device();

    /** @brief Runs the status handshake up to FEATURES_OK. */
    void negotiate_features();

    /** @brief Reads the 64-bit device feature set. */
    uint64_t read_device_features() const;

    /** 
     * @brief Writes the 64-bit driver feature set. 
     * @param features The 64-bit driver feature set to write.
     */
    void write_driver_features(uint64_t features);

    const int device_fd_;
    const PCIDevice& pci_device_;
    VfioHandle& vfio_handle_;

    volatile virtio_pci_common_cfg* common_cfg_ = nullptr;
    uint32_t common_cfg_length_ = 0;
    volatile uint8_t* isr_ = nullptr;
    volatile uint8_t* device_cfg_ = nullptr;
    volatile uint8_t* notify_base_ = nullptr;
    uint32_t notify_off_multiplier_ = 0;
    uint16_t current_queue_index_ = 0;
};
