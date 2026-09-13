#include "virtio_device.h"
#include "constants.h"
#include "utils.h"

#include <unistd.h>

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

    reset_device();
    negotiate_features();

    spdlog::info("VirtIO device status handshake complete (FEATURES_OK). Waiting on virtqueue setup before DRIVER_OK.");
}

VirtioDevice::~VirtioDevice()
{

}

void VirtioDevice::reset_device()
{
    common_cfg_->device_status = 0;

    constexpr int max_attempts = 1'000'000;
    int attempts = 0;
    while (common_cfg_->device_status != 0)
    {
        if (++attempts >= max_attempts)
        {
            LOG_AND_THROW("Timed out waiting for device reset to complete");
        }
    }

    spdlog::info("VirtIO device reset complete");
}

uint64_t VirtioDevice::read_device_features() const
{
    common_cfg_->device_feature_select = 0;
    const uint32_t features_low = common_cfg_->device_feature;
    common_cfg_->device_feature_select = 1;
    const uint32_t features_high = common_cfg_->device_feature;

    return (static_cast<uint64_t>(features_high) << 32) | features_low;
}

void VirtioDevice::write_driver_features(uint64_t features)
{
    common_cfg_->driver_feature_select = 0;
    common_cfg_->driver_feature = static_cast<uint32_t>(features);
    common_cfg_->driver_feature_select = 1;
    common_cfg_->driver_feature = static_cast<uint32_t>(features >> 32);
}

void VirtioDevice::negotiate_features()
{
    // 1. ACKNOWLEDGE: we have noticed the device.
    common_cfg_->device_status |= VIRTIO_STATUS_ACKNOWLEDGE;

    // 2. DRIVER: we know how to drive the device.
    common_cfg_->device_status |= VIRTIO_STATUS_DRIVER;

    // 3. Read what the device offers
    const uint64_t device_features = read_device_features();
    spdlog::info("Device offers features: 0x{:016x}", device_features);

    if (!(device_features & VIRTIO_F_VERSION_1_BIT))
    {
        common_cfg_->device_status |= VIRTIO_STATUS_FAILED;
        LOG_AND_THROW("Device does not offer VIRTIO_F_VERSION_1 (legacy-only devices are not supported)");
    }

    const uint64_t negotiated_features = device_features & VIRTIO_SUPPORTED_FEATURES;

    // 4. Tell the device which of its offered features we accept.
    write_driver_features(negotiated_features);

    // 5. FEATURES_OK: feature negotiation is complete.
    common_cfg_->device_status |= VIRTIO_STATUS_FEATURES_OK;

    // 6. Re-read status: the device MAY reject our feature subset by clearing FEATURES_OK.
    const uint8_t status = common_cfg_->device_status;
    if (!(status & VIRTIO_STATUS_FEATURES_OK))
    {
        common_cfg_->device_status |= VIRTIO_STATUS_FAILED;
        LOG_AND_THROW("Device rejected feature negotiation (FEATURES_OK was cleared)");
    }

    spdlog::info("Negotiated features: 0x{:016x}", negotiated_features);
}

void VirtioDevice::set_driver_ok()
{
    // 7. DRIVER_OK: the device is now "live" and may start processing virtqueues.
    common_cfg_->device_status |= VIRTIO_STATUS_DRIVER_OK;
    spdlog::info("VirtIO device is live (DRIVER_OK set)");
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

            if (virtio_cap.cfg_type == VIRTIO_PCI_CAP_PCI_CFG)
            {
                // bar, offset, and length here are a mutable config-space access window, not a real BAR region.
                // They should not be resolved through find_bar(). We already have every BAR memory-mapped directly, so this capability is simply unused.
                continue;
            }

            const mapped_bar* bar = pci_device_.find_bar(virtio_cap.bar_index);
            if (bar == nullptr)
            {
                spdlog::warn("VirtIO capability references BAR {} which is not mapped, skipping", virtio_cap.bar_index);
                continue;
            }

            if (static_cast<uint64_t>(virtio_cap.offset) + virtio_cap.length > bar->size)
            {
                LOG_AND_THROW("VirtIO capability region exceeds mapped BAR " + std::to_string(virtio_cap.bar_index) + " size");
            }

            uint8_t* bar_address = static_cast<uint8_t*>(bar->address) + virtio_cap.offset;

            switch (virtio_cap.cfg_type)
            {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                common_cfg_ = reinterpret_cast<volatile virtio_pci_common_cfg*>(bar_address);
                common_cfg_length_ = virtio_cap.length;
                break;

            case VIRTIO_PCI_CAP_NOTIFY_CFG:
            {
                uint32_t notify_off_multiplier = 0;
                off_t multiplier_pos = static_cast<off_t>(pci_device_.get_config_region().offset + cap.offset + sizeof(virtio_capability));
                ssize_t multiplier_ret = pread(device_fd_, &notify_off_multiplier, sizeof(notify_off_multiplier), multiplier_pos);
                if (multiplier_ret != static_cast<ssize_t>(sizeof(notify_off_multiplier)))
                {
                    LOG_AND_THROW("Failed to read notify_off_multiplier at 0x" + std::to_string(multiplier_pos));
                }

                notify_base_ = bar_address;
                notify_off_multiplier_ = notify_off_multiplier;
                break;
            }

            case VIRTIO_PCI_CAP_ISR_CFG:
                isr_ = bar_address;
                break;

            case VIRTIO_PCI_CAP_DEVICE_CFG:
                device_cfg_ = bar_address;
                break;

            case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
                spdlog::info("VirtIO shared memory region id={} at BAR{} offset=0x{:x} length=0x{:x} (unused by virtio-net)",
                    virtio_cap.id, virtio_cap.bar_index, virtio_cap.offset, virtio_cap.length);
                break;

            case VIRTIO_PCI_CAP_VNDR_CFG:
                spdlog::info("VirtIO vendor-specific config id={} at BAR{} offset=0x{:x} length=0x{:x} (unused by virtio-net)",
                    virtio_cap.id, virtio_cap.bar_index, virtio_cap.offset, virtio_cap.length);
                break;

            default:
                spdlog::warn("Unknown VirtIO cfg_type {}", virtio_cap.cfg_type);
                break;
            }
        }
    }

    if (common_cfg_ == nullptr || isr_ == nullptr || device_cfg_ == nullptr || notify_base_ == nullptr)
    {
        LOG_AND_THROW("Device is missing a required VirtIO PCI capability");
    }
}