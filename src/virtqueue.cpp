#include "virtqueue.h"
#include "vfio_handler.h"
#include "data_structures/virtqueue_layout.h"

Virtqueue::Virtqueue(VfioHandle& vfio_handle, uint16_t queue_size)
    : vfio_handle_(vfio_handle), queue_size_(queue_size)
{
    const uint64_t desc_size = static_cast<uint64_t>(queue_size_) * sizeof(virtq_desc);
    const uint64_t avail_size = sizeof(virtq_avail_header) + static_cast<uint64_t>(queue_size_) * sizeof(uint16_t);
    const uint64_t used_size = sizeof(virtq_used_header) + static_cast<uint64_t>(queue_size_) * sizeof(virtq_used_elem);

    desc_region_ = vfio_handle_.allocate_dma_memory(desc_size);
    avail_region_ = vfio_handle_.allocate_dma_memory(avail_size);
    used_region_ = vfio_handle_.allocate_dma_memory(used_size);
}

Virtqueue::~Virtqueue()
{
    vfio_handle_.free_dma_memory(desc_region_);
    vfio_handle_.free_dma_memory(avail_region_);
    vfio_handle_.free_dma_memory(used_region_);
}