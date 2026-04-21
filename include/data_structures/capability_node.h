#pragma once

#include <cstdint>

struct capability_node
{
    uint8_t cap_id;     // Capability ID (used to identify the type of capability)
    uint8_t next_ptr;   // Pointer to the next capability in the linked list
    uint8_t cap_len;    // Length of the capability structure
};