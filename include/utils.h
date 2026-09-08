#pragma once

#define LOG_AND_THROW(msg) \
    spdlog::error(msg); \
    throw std::runtime_error(msg);

// Rounds `value` up to the nearest multiple of `alignment`. `alignment` MUST be a power of 2.
#define ALIGN_MUL_CEIL(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))
