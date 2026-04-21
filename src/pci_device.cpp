#include "pci_device.h"
#include "utils.h"

#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vfio.h>
#include <sys/mman.h>

#include "spdlog/spdlog.h"

PCIDevice::PCIDevice(const int device_fd)
{
    read_config_header(device_fd);
    map_bar_regions(device_fd);
    read_capabilities(device_fd);
}

PCIDevice::~PCIDevice()
{
    // Unmap any mapped BAR regions
    for (const auto& bar : mapped_bars_)
    {
        if (bar.address != nullptr)
        {
            munmap(bar.address, bar.size);
        }
    }
}

void PCIDevice::read_config_header(const int device_fd)
{
    config_region_ = { .argsz = sizeof(config_region_), .index = VFIO_PCI_CONFIG_REGION_INDEX};
    
    if (ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &config_region_) < 0)
    {
        LOG_AND_THROW("Failed to get config region info");
    }

    spdlog::info("Config Space found at offset: 0x{0:x} (Size: 0x{1:x})", config_region_.offset, config_region_.size);

    // Now we read from config_region_.offset, NOT from 0
    ssize_t ret = pread(device_fd, &config_header_, sizeof(config_header_), config_region_.offset);

    if (ret != sizeof(config_header_))
    {
        LOG_AND_THROW("Failed to read full PCI header");
    }

    spdlog::info("PCI Config Header read successfully!");
}

void PCIDevice::map_bar_regions(const int device_fd)
{
    for (unsigned int bar_index = VFIO_PCI_BAR0_REGION_INDEX; bar_index < VFIO_PCI_BAR5_REGION_INDEX; bar_index++)
    {
        vfio_region_info reg = { .argsz = sizeof(reg), .index = bar_index };

        if (ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0)
        {
            LOG_AND_THROW("Failed to get region info for BAR " + std::to_string(bar_index));
        }

        if (reg.size > 0 && (reg.flags & VFIO_REGION_INFO_FLAG_MMAP))
        {
            spdlog::info("Mapping BAR {0}: Size=0x{1:x}, Offset=0x{2:x}", bar_index, reg.size, reg.offset);

            void* mapped_addr = mmap(nullptr, reg.size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, reg.offset);
            if (mapped_addr == MAP_FAILED)
            {
                LOG_AND_THROW("Failed to mmap BAR " + std::to_string(bar_index));
            }

            mapped_bars_.emplace_back(mapped_addr, reg.size, reg.flags);
        }
    }
}

void PCIDevice::read_capabilities(const int device_fd)
{
    spdlog::info("Walking PCI capability list starting at 0x{0:x}", config_header_.capabilities_pointer);

    uint8_t cap_ptr = config_header_.capabilities_pointer;
    
    while (cap_ptr != 0)
    {
        capability_node cap_node;

        if (cap_ptr + sizeof(capability_node) > config_region_.size)
        {
            throw std::runtime_error("Config read out of bounds");
        }

        off_t pos = static_cast<off_t>(config_region_.offset + cap_ptr);
        ssize_t ret = pread(device_fd, &cap_node, sizeof(capability_node), pos);
        if (ret != static_cast<ssize_t>(sizeof(capability_node)))
        {
            LOG_AND_THROW("Failed to read capability ID at 0x" + std::to_string(pos));
        }

        if (cap_node.next_ptr == cap_ptr)
        {
            spdlog::warn("Capability next pointer loops to itself at 0x{0:x}. Stopping traversal.", cap_ptr);
            break;
        }

        spdlog::info("Found capability at 0x{0:x}: ID=0x{1:x}, Next=0x{2:x}, Len={3}", cap_ptr, cap_node.cap_id, cap_node.next_ptr, cap_node.cap_len);

        capabilities_.push_back(cap_node);

        // Move to the next capability in the list
        cap_ptr = cap_node.next_ptr;
    }
}
