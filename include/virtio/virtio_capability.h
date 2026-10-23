#pragma once

#include <cstdint>

#include "pcie/capability.h"

struct virtio_capability
{
    pci_capability_node node;   // The standard capability node header
    uint8_t cfg_type;       // VirtIO-specific field to indicate the type of configuration
    uint8_t bar_index;      // BAR index where the capability's associated registers are located
    uint8_t id;             // Multiple capabilities of the same type can be distinguished by this ID field
    uint16_t padding;       // Padding to align the structure to a 4-byte boundary
    uint32_t offset;        // Offset of the capability's register block within the BAR
    uint32_t length;        // Length of the capability's register block
};