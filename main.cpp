#include "pci_device.h"
#include "vfio_handler.h"
#include "virtio_device.h"

#include "spdlog/spdlog.h"

#define IOMMU_GROUP_NUM 6
#define DEVICE_BDF "0000:01:00.0"

int main()
{
    VfioHandle vfio_handler(DEVICE_BDF, IOMMU_GROUP_NUM);
    PCIDevice pci_device(vfio_handler.get_device_fd());
    VirtioDevice virtio_device(vfio_handler.get_device_fd(), pci_device);



    return 0;
}