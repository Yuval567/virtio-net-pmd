#pragma once

#include <memory>

#include "pci_device.h"
#include "vfio_handler.h"
#include "virtqueue.h"
#include "data_structures/virtio_pci_cfg.h"

class VirtioDevice
{
public:
    VirtioDevice(const int device_fd, const PCIDevice& pci_device, VfioHandle& vfio_handle);
    ~VirtioDevice();

private:
    void initialize_capabilites();
    void reset_device();
    void negotiate_features();
    void setup_virtqueues();
    void setup_virtqueue(uint16_t queue_index, std::unique_ptr<Virtqueue>& queue_slot);
    void set_driver_ok();

    uint64_t read_device_features() const;
    void write_driver_features(uint64_t features);

    const int device_fd_;
    const PCIDevice& pci_device_;
    VfioHandle& vfio_handle_;

    volatile virtio_pci_common_cfg* common_cfg_ = nullptr;
    uint32_t common_cfg_length_ = 0;
    volatile uint8_t* isr_ = nullptr;
    volatile uint8_t* device_cfg_ = nullptr;
    volatile uint8_t* notify_base_ = nullptr;
    uint32_t notify_off_multiplier_ = 0;

    std::unique_ptr<Virtqueue> rx_queue_;
    std::unique_ptr<Virtqueue> tx_queue_;
};