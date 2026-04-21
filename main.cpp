#include <iostream>

#include "pci_device.h"
#include "vfio_handler.h"
#include "constants.h"

#include "spdlog/spdlog.h"

#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

int main()
{
    VfioHandle vfio_handler(DEVICE_BDF, IOMMU_GROUP_NUM);
    PCIDevice pci_device(vfio_handler.get_device_fd());

    spdlog::info("Vendor ID: 0x{:x}", pci_device.get_config_header().vendor_id);
    spdlog::info("Device ID: 0x{:x}", pci_device.get_config_header().device_id);

    if (pci_device.get_config_header().vendor_id == VIRTIO_VENDOR_ID && pci_device.get_config_header().device_id == VIRTIO_NET_DEVICE_ID)
    {
        spdlog::info("MATCH! We are talking to the VirtIO Net device.");
    }
    else
    {
        spdlog::warn("Warning: IDs do not match expected VirtIO values.");
    }

    return 0;
}