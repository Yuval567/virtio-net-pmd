#include "virtqueue.h"
#include "vfio_handler.h"
#include "constants.h"

#include <atomic>

Virtqueue::Virtqueue(VfioHandle& vfio_handle, uint16_t queue_size)
    : vfio_handle_(vfio_handle), queue_size_(queue_size)
{
    const uint64_t desc_size = static_cast<uint64_t>(queue_size_) * sizeof(virtq_desc);
    const uint64_t avail_size = sizeof(virtq_avail_header) + static_cast<uint64_t>(queue_size_) * sizeof(uint16_t);
    const uint64_t used_size = sizeof(virtq_used_header) + static_cast<uint64_t>(queue_size_) * sizeof(virtq_used_elem);
    const uint64_t buffer_size = static_cast<uint64_t>(queue_size_) * VIRTQUEUE_BUFFER_SIZE;

    // mmap'd anonymous memory (see VfioHandle::allocate_dma_memory) is zero-filled by the
    // kernel, which is exactly the state the spec requires before enabling a queue: avail.idx
    // and used.idx both start at 0.
    desc_region_ = vfio_handle_.allocate_dma_memory(desc_size);
    avail_region_ = vfio_handle_.allocate_dma_memory(avail_size);
    used_region_ = vfio_handle_.allocate_dma_memory(used_size);
    buffer_region_ = vfio_handle_.allocate_dma_memory(buffer_size);
}

Virtqueue::~Virtqueue()
{
    vfio_handle_.free_dma_memory(desc_region_);
    vfio_handle_.free_dma_memory(avail_region_);
    vfio_handle_.free_dma_memory(used_region_);
    vfio_handle_.free_dma_memory(buffer_region_);
}

void* Virtqueue::buffer_address(uint16_t slot) const
{
    return static_cast<uint8_t*>(buffer_region_.address) + static_cast<uint64_t>(slot) * VIRTQUEUE_BUFFER_SIZE;
}

uint64_t Virtqueue::buffer_iova(uint16_t slot) const
{
    return buffer_region_.iova + static_cast<uint64_t>(slot) * VIRTQUEUE_BUFFER_SIZE;
}

volatile virtq_desc* Virtqueue::desc_table() const
{
    return reinterpret_cast<volatile virtq_desc*>(desc_region_.address);
}

volatile virtq_avail_header* Virtqueue::avail_header() const
{
    return reinterpret_cast<volatile virtq_avail_header*>(avail_region_.address);
}

volatile uint16_t* Virtqueue::avail_ring() const
{
    return reinterpret_cast<volatile uint16_t*>(static_cast<uint8_t*>(avail_region_.address) + sizeof(virtq_avail_header));
}

volatile virtq_used_header* Virtqueue::used_header() const
{
    return reinterpret_cast<volatile virtq_used_header*>(used_region_.address);
}

volatile virtq_used_elem* Virtqueue::used_ring() const
{
    return reinterpret_cast<volatile virtq_used_elem*>(static_cast<uint8_t*>(used_region_.address) + sizeof(virtq_used_header));
}

void Virtqueue::publish(uint16_t slot, uint32_t len, uint16_t flags)
{
    volatile virtq_desc& d = desc_table()[slot];
    d.addr = buffer_iova(slot);
    d.len = len;
    d.flags = flags;
    d.next = 0;

    const uint16_t avail_idx = avail_header()->idx;
    avail_ring()[avail_idx % queue_size_] = slot;

    // The descriptor and avail ring entry above MUST be visible to the device before it
    // observes the idx bump below - otherwise it could read a half-written descriptor.
    // We don't negotiate VIRTIO_F_ORDER_PLATFORM, so a compiler barrier is sufficient on x86
    // (DMA is cache-coherent and the architecture doesn't reorder stores).
    std::atomic_thread_fence(std::memory_order_release);

    avail_header()->idx = avail_idx + 1;
}

bool Virtqueue::has_completed() const
{
    return used_header()->idx != last_used_idx_;
}

virtq_used_elem Virtqueue::take_completed()
{
    // Pairs with the release fence in publish(): makes sure we don't read the used ring
    // entry's fields until after we've observed the idx bump that made it valid.
    std::atomic_thread_fence(std::memory_order_acquire);

    const virtq_used_elem elem = { used_ring()[last_used_idx_ % queue_size_].id, used_ring()[last_used_idx_ % queue_size_].len };
    ++last_used_idx_;
    return elem;
}
