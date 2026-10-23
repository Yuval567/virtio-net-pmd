#pragma once

#include <vector>
#include <linux/vfio.h>

#include "pcie/pci_config_header.h"
#include "pcie/mapped_bar.h"
#include "pcie/capability.h"

/**
 * @brief A PCI device accessed through VFIO.
 *
 * Reads the config header and capability list, enables bus mastering and mmaps the BARs.
 */
class PCIDevice
{
public:
    /**
     * @param device_fd VFIO device fd.
     * @throws std::runtime_error if reading config space or mapping a BAR fails.
     */
    PCIDevice(const int device_fd);

    /** @brief Unmaps the BARs. */
    ~PCIDevice();

    const pci_config_header_type0& get_config_header() const noexcept { return config_header_; }
    const std::vector<mapped_bar>& get_mapped_bars() const noexcept { return mapped_bars_; }
    const std::vector<pci_capability>& get_capabilities() const noexcept { return capabilities_; }
    const vfio_region_info& get_config_region() const noexcept { return config_region_; }

    /** 
     * @brief Finds a mapped BAR by index. 
     * @param bar_index The index of the BAR to find.
     * @return The BAR, or nullptr if it isn't mapped. 
     */
    const mapped_bar* find_bar(uint8_t bar_index) const noexcept;

private:
    /** 
     * @brief Reads the type 0 config header. 
     * @param device_fd The VFIO device file descriptor.
     */
    void read_config_header(const int device_fd);

    /** 
     * @brief Sets the memory space and bus master bits in the command register. 
     * @param device_fd The VFIO device file descriptor.
     */
    void enable_bus_mastering(const int device_fd);

    /** 
     * @brief mmaps every BAR that supports it. 
     * @param device_fd The VFIO device file descriptor.
     */
    void map_bar_regions(const int device_fd);

    /** 
     * @brief Walks the capability linked list. 
     * @param device_fd The VFIO device file descriptor.
     */
    void read_capabilities(const int device_fd);

    vfio_region_info config_region_;
    pci_config_header_type0 config_header_;
    std::vector<mapped_bar> mapped_bars_;
    std::vector<pci_capability> capabilities_;
};
