#pragma once

#include <cstdint>

/** @brief Descriptor flags. */
constexpr uint16_t VIRTQ_DESC_F_NEXT = 1;
constexpr uint16_t VIRTQ_DESC_F_WRITE = 2;
constexpr uint16_t VIRTQ_DESC_F_INDIRECT = 4;

/** @brief One entry of the descriptor table, describes a single buffer. */
struct virtq_desc
{
    /** @brief IOVA of the buffer. */
    uint64_t addr;
    
    /** @brief Buffer length in bytes. */
    uint32_t len;
    
    /** @brief Descriptor flags. */
    uint16_t flags;
    
    /** @brief Next descriptor in the chain, valid only with VIRTQ_DESC_F_NEXT. */
    uint16_t next;
};

/** @brief Header of the avail ring (driver -> device). The ring entries follow it. */
struct virtq_avail_header
{
    /** @brief Driver hints to the device, e.g. to suppress interrupts. */
    uint16_t flags;

    /** @brief Where the driver writes the next entry. Free-running, wraps at 2^16. */
    uint16_t idx;
};

/** @brief One entry of the used ring, describes a buffer the device is done with. */
struct virtq_used_elem
{
    /** @brief Index of the descriptor chain head that was used. */
    uint32_t id;

    /** @brief Total bytes the device wrote into that chain. */
    uint32_t len;
};

/** @brief Header of the used ring (device -> driver). The ring entries follow it. */
struct virtq_used_header
{
    /** @brief Device hints to the driver, e.g. to suppress notifications. */
    uint16_t flags;

    /** @brief Where the device writes the next entry. Free-running, wraps at 2^16. */
    uint16_t idx;
};
