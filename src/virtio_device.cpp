#include "virtio_device.h"
#include "constants.h"
#include "utils.h"

#include <spdlog/spdlog.h>

VirtioDevice::VirtioDevice(const int device_fd, const PCIDevice& pci_device)
    : device_fd_(device_fd), pci_device_(pci_device)
{
    spdlog::info("Vendor ID: 0x{:x}", pci_device.get_config_header().vendor_id);
    spdlog::info("Device ID: 0x{:x}", pci_device.get_config_header().device_id);

    if (pci_device.get_config_header().vendor_id == VIRTIO_VENDOR_ID && pci_device.get_config_header().device_id == VIRTIO_NET_DEVICE_ID)
    {
        spdlog::info("MATCH! We are talking to the VirtIO Net device.");
    }
    else
    {
        LOG_AND_THROW("Device does not match expected VirtIO Net device.");
    }

    initialize_capabilites();
}

VirtioDevice::~VirtioDevice()
{

}

void VirtioDevice::initialize_capabilites()
{
    for (const auto& cap : pci_device_.get_capabilities())
    {
        if (cap.node.cap_id == VIRTIO_VENDOR_SPECIFIC_CAP_ID)
        {
            virtio_capability virtio_cap;

            if (cap.offset + sizeof(virtio_capability) > pci_device_.get_config_region().size)
            {
                throw std::runtime_error("Config read out of bounds");
            }

            off_t pos = static_cast<off_t>(pci_device_.get_config_region().offset + cap.offset);
            ssize_t ret = pread(device_fd_, &virtio_cap, sizeof(virtio_capability), pos);
            if (ret != static_cast<ssize_t>(sizeof(virtio_capability)))
            {
                LOG_AND_THROW("Failed to read capability ID at 0x" + std::to_string(pos));
            }

            spdlog::info("VirtIO Capability Details: cfg_type={}, bar_index={}, id={}, offset=0x{:x}, length=0x{:x}",
                virtio_cap.cfg_type, virtio_cap.bar_index, virtio_cap.id, virtio_cap.offset, virtio_cap.length);
        }
    }
}