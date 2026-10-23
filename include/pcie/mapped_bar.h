#pragma once

#include <cstdint>

struct mapped_bar
{
    void* address;      // Pointer to the mapped BAR region
    uint64_t size;      // Size of the mapped region
    uint32_t flags;     // Flags indicating the type of BAR (e.g., memory-mapped, I/O-mapped)
    uint32_t bar_index; // PCI BAR index this region was mapped from
};