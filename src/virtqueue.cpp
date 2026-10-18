#include "virtqueue.h"
#include "vfio_handler.h"
#include "data_structures/virtqueue_layout.h"

#include <atomic>

Virtqueue::Virtqueue(VfioHandle& vfio_handle, uint16_t queue_index, uint16_t queue_size, volatile uint16_t* notify_addr)
    : vfio_handle_(vfio_handle), queue_size(queue_size), notify_addr_(notify_addr), queue_index(queue_index)
{
    const uint64_t desc_size = static_cast<uint64_t>(queue_size) * sizeof(virtq_desc);
    const uint64_t avail_size = sizeof(virtq_avail_header) + static_cast<uint64_t>(queue_size) * sizeof(uint16_t);
    const uint64_t used_size = sizeof(virtq_used_header) + static_cast<uint64_t>(queue_size) * sizeof(virtq_used_elem);

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

void Virtqueue::notify() const noexcept
{
    *notify_addr_ = queue_index;
}