#pragma once

#include <cstdint>

// PCI Local Bus spec: bits of the 16-bit command register (pci_config_header_type0::command).
constexpr uint16_t PCI_COMMAND_IO_SPACE = 0x1;     // enables responding to I/O space accesses
constexpr uint16_t PCI_COMMAND_MEMORY_SPACE = 0x2; // enables responding to memory space accesses (BARs)
constexpr uint16_t PCI_COMMAND_BUS_MASTER = 0x4;   // lets the device initiate its own DMA reads/writes

struct pci_config_header_type0
{
    uint16_t vendor_id;                     // Vendor ID
    uint16_t device_id;                     // Device ID
    uint16_t command;                       // Command register (used to enable bus mastering, memory access, etc.)
    uint16_t status;                        // Status register (contains various status bits)
    uint8_t revision_id;                    // Revision ID (device specific)
    uint8_t  class_code[3];                 // Class Code (used to identify the type of device, e.g. network controller, storage controller, etc.)
    uint8_t  cache_line_size;               // Cache Line Size (used for optimizing memory accesses) 
    uint8_t latency_timer;                  // Latency timer (used for bus arbitration timing)
    uint8_t header_type;                    // Header type (indicates the layout of the rest of the header)
    uint8_t bist;                           // Built-in self-test (BIST) (used for device diagnostics)
    uint32_t bars[6];                       // Base Address Registers (BARs) (used to map device memory or I/O ports)
    uint32_t cardbus_cis_pointer;           // CardBus CIS Pointer (used for CardBus devices)
    uint16_t subsystem_vendor_id;           // Subsystem Vendor ID (used to identify the specific implementation of the device)
    uint16_t subsystem_id;                  // Subsystem ID (used to identify the specific implementation of the device)
    uint32_t expansion_rom_base_address;    // Expansion ROM Base Address (used to map the device's expansion ROM)
    uint8_t capabilities_pointer;           // Capabilities Pointer (used to point to the first capability in the linked list of capabilities)
    uint8_t reserved[7];                    // Reserved bytes
    uint8_t interrupt_line;                 // Interrupt Line (used to specify which interrupt line the device uses)
    uint8_t interrupt_pin;                  // Interrupt Pin (used to specify which interrupt pin the device uses)
    uint8_t min_grant;                      // Minimum Grant (used for bus arbitration)
    uint8_t max_latency;                    // Maximum Latency (used for bus arbitration)
};