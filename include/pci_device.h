#pragma once

#include <vector>
#include <linux/vfio.h>

#include "data_structures/pci_config_header.h"
#include "data_structures/mapped_bar.h"
#include "data_structures/capability.h"

class PCIDevice
{
public:
    PCIDevice(const int device_fd);
    ~PCIDevice();

    const pci_config_header_type0& get_config_header() const noexcept { return config_header_; }
    const std::vector<mapped_bar>& get_mapped_bars() const noexcept { return mapped_bars_; }
    const std::vector<pci_capability>& get_capabilities() const noexcept { return capabilities_; }
    const vfio_region_info& get_config_region() const noexcept { return config_region_; }

    const mapped_bar* find_bar(uint8_t bar_index) const noexcept;

private:
    void read_config_header(const int device_fd);
    void map_bar_regions(const int device_fd);
    void read_capabilities(const int device_fd);

    vfio_region_info config_region_;
    pci_config_header_type0 config_header_;
    std::vector<mapped_bar> mapped_bars_;
    std::vector<pci_capability> capabilities_;
};