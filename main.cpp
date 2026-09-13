#include "pci_device.h"
#include "vfio_handler.h"
#include "virtio_device.h"

#include "spdlog/spdlog.h"

#define DEVICE_BDF "0000:00:05.0"

int main()
{
    VfioHandle vfio_handler(DEVICE_BDF);
    PCIDevice pci_device(vfio_handler.get_device_fd());
    VirtioDevice virtio_device(vfio_handler.get_device_fd(), pci_device, vfio_handler);

    return 0;
}