#include "pci_device.h"
#include "utils.h"

#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vfio.h>

#include "spdlog/spdlog.h"

PCIDevice::PCIDevice(const int device_fd)
{
    // Read the PCI configuration header from the device
    read_config_header(device_fd);
}

PCIDevice::~PCIDevice()
{}

void PCIDevice::read_config_header(const int device_fd)
{
    // Step 6: Find Config Space Region Info

    vfio_region_info reg = { .argsz = sizeof(reg) };
    reg.index = VFIO_PCI_CONFIG_REGION_INDEX;

    if (ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &reg) < 0)
    {
        LOG_AND_THROW("Failed to get config region info");
    }

    spdlog::info("Config Space found at offset: 0x{0:x} (Size: 0x{1:x})", reg.offset, reg.size);



    // Now we read from reg.offset, NOT from 0
    ssize_t ret = pread(device_fd, &config_header_, sizeof(config_header_), reg.offset);

    if (ret != sizeof(config_header_))
    {
        LOG_AND_THROW("Failed to read full PCI header");
    }

    spdlog::info("PCI Config Header read successfully!");
}

