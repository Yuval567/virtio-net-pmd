#pragma once

#include <cstdint>

/** @brief PCI capability ID for vendor-specific capabilities. */
constexpr uint8_t PCI_CAP_ID_VENDOR_SPECIFIC = 0x09;

struct pci_capability_node
{
    /** @brief PCI capability ID. */
    uint8_t cap_id;

    /** @brief Next capability pointer. */
    uint8_t next_ptr;

    /** @brief Length of the capability structure. */
    uint8_t cap_len;
};

struct pci_capability
{
    /** @brief Offset of the capability in the PCI configuration space. */
    uint64_t offset;
    
    /** @brief The capability node data. */
    pci_capability_node node;    
};
