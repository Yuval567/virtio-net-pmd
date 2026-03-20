#include "pci_device.h"

#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vfio.h>

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
        throw std::runtime_error("Failed to get config region info");
    }

    std::cout << "Config Space found at offset: 0x" << std::hex << reg.offset
        << " (Size: 0x" << reg.size << ")" << std::endl;



    // Now we read from reg.offset, NOT from 0
    ssize_t ret = pread(device_fd, &config_header_, sizeof(config_header_), reg.offset);

    if (ret != sizeof(config_header_))
    {
        throw std::runtime_error("Failed to read full PCI header");
    }

    std::cout << "PCI Config Header read successfully!" << std::endl;
}

