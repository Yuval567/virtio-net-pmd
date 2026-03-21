#pragma once

#define LOG_AND_THROW(msg) \
    spdlog::error(msg); \
    throw std::runtime_error(msg);
