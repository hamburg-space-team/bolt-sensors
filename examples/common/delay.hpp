#pragma once

#include <cstdint>

#ifdef BUILD_TARGET_HARDWARE
#include "main.h" // IWYU pragma: keep
#else
#include <chrono>
#include <thread>
#endif

inline static void delay(uint32_t ms) noexcept {
#if defined(BUILD_TARGET_HARDWARE) && defined(USE_HAL_DRIVER)
    HAL_Delay(ms);
#elif defined(BUILD_TARGET_HARDWARE)
    LL_mDelay(ms); // LL only: needs SysTick at 1 ms
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}
