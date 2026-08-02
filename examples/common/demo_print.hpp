#pragma once

#include <cstdio>

#ifdef BUILD_TARGET_HARDWARE

#include <algorithm>
#include <array>
#include <cstdint>

using HardwarePrintCallback = void (*)(const uint8_t* data, std::size_t length);

inline HardwarePrintCallback g_print_callback = nullptr;

inline void demo_print_init(HardwarePrintCallback callback) {
    g_print_callback = callback;
}

template <typename... Args> void demo_print(const char* fmt, Args... args) {
    if (!g_print_callback) {
        return;
    }

    std::array<char, 128> buffer{};
    int num_bytes = 0;

    if constexpr (sizeof...(Args) == 0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        num_bytes = std::snprintf(buffer.data(), buffer.size(), "%s", fmt);
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        num_bytes = std::snprintf(buffer.data(), buffer.size(), fmt, args...);
    }

    if (num_bytes > 0) {
        const auto len = std::min<std::size_t>(num_bytes, buffer.size() - 1U);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        g_print_callback(reinterpret_cast<const uint8_t*>(buffer.data()), len);
    }
}

template <typename... Args> void demo_println(const char* fmt, Args... args) {
    demo_print(fmt, args...);

    if (g_print_callback) {
        const std::array<uint8_t, 2> crlf = {'\r', '\n'};
        g_print_callback(crlf.data(), crlf.size());
    }
}

#else

template <typename... Args> void demo_print(const char* fmt, Args... args) {
    if constexpr (sizeof...(args) == 0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        std::printf("%s", fmt);
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        std::printf(fmt, args...);
    }
}

template <typename... Args> void demo_println(const char* fmt, Args... args) {
    demo_print(fmt, args...);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::printf("\n");
}

#endif
