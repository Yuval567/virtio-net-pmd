#pragma once

#include <cstdint>

/**
 * @brief Memory mapped for device DMA.
 */
struct dma_memory_region
{
    /** @brief Virtual address of the region. */
    void* address;

    /** @brief IOVA of the region, as the device sees it. */
    uint64_t iova;

    /** @brief Size of the region in bytes. */
    uint64_t size;
};
