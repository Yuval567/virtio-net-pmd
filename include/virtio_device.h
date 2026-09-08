#pragma once

#include "pci_device.h"
#include "data_structures/virtio_pci_cfg.h"

class VirtioDevice
{
public:
    VirtioDevice(const int device_fd, const PCIDevice& pci_device);
    ~VirtioDevice();

private:
    void initialize_capabilites();

    const int device_fd_;
    const PCIDevice& pci_device_;

    virtio_pci_common_cfg* common_cfg_ = nullptr;
    uint32_t common_cfg_length_ = 0;
    volatile uint8_t* isr_ = nullptr;
    uint8_t* device_cfg_ = nullptr;
    uint8_t* notify_base_ = nullptr;
    uint32_t notify_off_multiplier_ = 0;
};