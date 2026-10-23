#pragma once

#include <string>
#include <cstdint>

#include "common/dma_memory_region.h"

/** @brief Owns the VFIO container, group and device fds for one PCI device. */
class VfioHandle
{
public:
    /**
     * @param bdf PCI address of the device
     * @throws std::runtime_error if any VFIO setup step fails.
     */
    explicit VfioHandle(const std::string& bdf);

    /** @brief Closes the device, group and container fds. */
    ~VfioHandle();

    int get_device_fd() const noexcept { return device_fd_; }

    /**
     * @brief Allocates page-aligned memory and maps it for device DMA (IOVA == vaddr).
     * @param size Requested size in bytes, rounded up to a page.
     * @throws std::runtime_error if allocation or mapping fails. 
     * @return The allocated region.
     */
    dma_memory_region allocate_dma_memory(uint64_t size);

    /** 
     * @brief Unmaps and releases a region from allocate_dma_memory().
     * @param memory The region to free.
     * @throws std::runtime_error if unmapping or releasing fails.
     */
    void free_dma_memory(const dma_memory_region& memory);

private:
    /** @brief Opens /dev/vfio/vfio and checks API version and Type1 IOMMU support. */
    void open_container();

    /** @brief Opens the IOMMU group and attaches it to the container. */
    void open_group(int iommu_group);

    /** @brief Gets the device fd from the group. */
    void get_device_fd(const std::string& bdf);

    /** @brief Looks up the IOMMU group number of a device through sysfs. */
    static int get_iommu_group(const std::string& bdf);

    int container_fd_;
    int group_fd_;
    int device_fd_;
};
