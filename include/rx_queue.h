#pragma once

#include <cstdint>

#include "virtqueue.h"

class RxQueue : public Virtqueue
{
public:
    RxQueue(VfioHandle& vfio_handle, uint16_t queue_index, uint16_t queue_size, volatile uint16_t* notify_addr)
        : Virtqueue(vfio_handle, queue_index, queue_size, notify_addr) {}

    RxQueue(const RxQueue&) = delete;
    RxQueue& operator=(const RxQueue&) = delete;

    
}