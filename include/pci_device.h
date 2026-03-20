#pragma once

#include "data_structures/pci_config_header.h"


class PCIDevice
{
public:
    PCIDevice(const int device_fd);
    ~PCIDevice();

    const pci_config_header& get_config_header() const noexcept { return config_header_; }

private:
    void read_config_header(const int device_fd);
    void map_bar_regions(const int device_fd);

    pci_config_header config_header_;
};