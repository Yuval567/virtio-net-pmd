#pragma once

/**
 * @brief Logs msg as an error and throws std::runtime_error with it.
 */
#define LOG_AND_THROW(msg) \
    spdlog::error(msg); \
    throw std::runtime_error(msg);

/** @brief Rounds value up to a multiple of alignment. alignment MUST be a power of 2. */
#define ALIGN_MUL_CEIL(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))
