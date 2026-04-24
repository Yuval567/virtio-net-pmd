#pragma once

#include "pci_device.h"

class VirtioDevice
{
public:
    VirtioDevice(const int device_fd, const PCIDevice& pci_device);
    ~VirtioDevice();

private:
    void initialize_capabilites();

    const int device_fd_;
    const PCIDevice& pci_device_;
};