#include <iostream>

#include "pci_device.h"
#include "vfio_handler.h"

#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

int main()
{
    VfioHandle vfio_handler(DEVICE_BDF, IOMMU_GROUP_NUM);
    PCIDevice pci_device(vfio_handler.get_device_fd());

    std::cout << "Vendor ID: 0x" << std::hex << pci_device.get_config_header().vendor_id << std::endl;
    std::cout << "Device ID: 0x" << std::hex << pci_device.get_config_header().device_id << std::endl;

    if (pci_device.get_config_header().vendor_id == 0x1af4 && pci_device.get_config_header().device_id == 0x1041)
    {
        std::cout << "MATCH! We are talking to the VirtIO Net device." << std::endl;
    }
    else
    {
        std::cout << "Warning: IDs do not match expected VirtIO values." << std::endl;
    }

    return 0;
}