#pragma once

#include <cstdint>

// PCI capability ID of the vendor-specific capability (virtio uses it to expose its config structures).
constexpr uint8_t PCI_CAP_ID_VENDOR_SPECIFIC = 0x09;

struct pci_capability_node
{
    uint8_t cap_id;     // Capability ID (used to identify the type of capability)
    uint8_t next_ptr;   // Pointer to the next capability in the linked list
    uint8_t cap_len;    // Length of the capability structure
};

struct pci_capability
{
    uint64_t offset;        // Offset of the capability in the PCI configuration space
    pci_capability_node node;   // The capability node data    
};
