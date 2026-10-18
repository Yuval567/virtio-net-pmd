#pragma once

#include <memory>

#include "pci_device.h"
#include "vfio_handler.h"
#include "virtqueue.h"
#include "data_structures/virtio_pci_cfg.h"

class VirtioDevice
{
public:
    VirtioDevice(const int device_fd, const PCIDevice& pci_device, VfioHandle& vfio_handle);
    
    VirtioDevice(const VirtioDevice&) = delete;
    VirtioDevice& operator=(const VirtioDevice&) = delete;

    std::unique_ptr<Virtqueue> create_virtqueue();

    void start();

private:
    void initialize_capabilites();
    void reset_device();
    void negotiate_features();

    uint64_t read_device_features() const;
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