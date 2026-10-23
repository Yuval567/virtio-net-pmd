#pragma once

#include <cstdint>

/** @brief Represents a memory-mapped PCI BAR (Base Address Register) region. */
struct mapped_bar
{
    /** @brief Pointer to the mapped BAR region. */
    void* address;

    /** @brief Size of the mapped region. */
    uint64_t size;

    /** @brief Flags indicating the type of BAR (e.g., memory-mapped, I/O-mapped). */
    uint32_t flags;

    /** @brief PCI BAR index this region was mapped from. */
    uint32_t bar_index;
};