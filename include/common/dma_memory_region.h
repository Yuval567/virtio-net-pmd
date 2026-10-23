#pragma once

#include <cstdint>

// A region of process memory mapped for device DMA. `iova` is the address the
// device must use to reach this memory - identity-mapped to the process virtual
// address here, since we only ever deal with a handful of small, non-overlapping
// regions and a real IOVA allocator isn't needed at this scale.
struct dma_memory_region
{
    void* address;
    uint64_t iova;
    uint64_t size;
};
