#pragma once

#include <cstdint>

#include "pcie/capability.h"

/**
 * @brief Virtio PCI capability structure.
 *
 * This structure represents a Virtio-specific PCI capability, which is used to
 * describe various configuration and control registers for Virtio devices.
 */
struct virtio_capability
{
    /** @brief Standard PCI capability node header. */
    pci_capability_node node;
    
    /** @brief Indicates the type of Virtio capability. */
    uint8_t cfg_type;
    
    /** @brief The BAR index where the capability's associated registers are located. */
    uint8_t bar_index;
    
    /** @brief A unique identifier for the capability. */
    uint8_t id;
    
    /** @brief Padding to align the structure to a 4-byte boundary. */
    uint16_t padding;
    
    /** @brief The offset of the capability's register block within the BAR. */
    uint32_t offset;
    
    /** @brief The length of the capability's register block. */
    uint32_t length;
};