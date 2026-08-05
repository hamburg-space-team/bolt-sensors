#pragma once

#include <cstdint>

#ifdef BUILD_TARGET_HARDWARE
#include "main.h" // IWYU pragma: keep
#else
#include <chrono>
#include <thread>
#endif

inline static void delay(uint32_t ms) {
#ifdef BUILD_TARGET_HARDWARE
    HAL_Delay(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}
