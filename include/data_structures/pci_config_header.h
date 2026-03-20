#pragma once

#include <cstdint>

struct pci_config_header
{
    uint16_t vendor_id;                     // Vendor ID (0x1af4 for VirtIO)
    uint16_t device_id;                     // Device ID (0x1041 for VirtIO Net)
    uint16_t command;                       // Command register (used to enable bus mastering, memory access, etc.)
    uint16_t status;                        // Status register (contains various status bits)
    uint8_t revision_id;                    // Revision ID (device-specific)
    uint8_t latency_timer;                  // Latency timer (used for bus arbitration)
    uint8_t header_type;                    // Header type (indicates the layout of the rest of the header)
    uint8_t bist;                           // Built-in self-test (BIST) (used for device diagnostics)
    uint32_t bars[6];                       // Base Address Registers (BARs) (used to map device memory or I/O ports)
    uint32_t cardbus_cis_pointer;           // CardBus CIS Pointer (used for CardBus devices, typically not relevant for VirtIO)
    uint16_t subsystem_vendor_id;           // Subsystem Vendor ID (used to identify the specific implementation of the device, )
    uint16_t subsystem_id;                  // Subsystem ID (used to identify the specific implementation of the device, )
    uint32_t expansion_rom_base_address;    // Expansion ROM Base Address (used to map the device's expansion ROM, if present)
    uint8_t capabilities_pointer;           // Capabilities Pointer (used to point to the first capability in the linked list of capabilities)
};
