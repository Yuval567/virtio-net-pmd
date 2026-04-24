#pragma once

#include <cstdint>

struct capability_node
{
    uint8_t cap_id;     // Capability ID (used to identify the type of capability)
    uint8_t next_ptr;   // Pointer to the next capability in the linked list
    uint8_t cap_len;    // Length of the capability structure
};

struct pci_capability
{
    uint64_t offset;        // Offset of the capability in the PCI configuration space
    capability_node node;   // The capability node data    
};

struct virtio_capability
{
    capability_node node;   // The standard capability node header
    uint8_t cfg_type;       // VirtIO-specific field to indicate the type of configuration
    uint8_t bar_index;      // BAR index where the capability's associated registers are located
    uint8_t id;             // Multiple capabilities of the same type can be distinguished by this ID field
    uint16_t padding;       // Padding to align the structure to a 4-byte boundary
    uint32_t offset;        // Offset of the capability's register block within the BAR
    uint32_t length;        // Length of the capability's register block
};