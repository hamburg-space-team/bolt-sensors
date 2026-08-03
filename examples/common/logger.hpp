#pragma once

#include <cstdio>

#ifdef BUILD_TARGET_HARDWARE
#include <algorithm>
#include <array>
#include <cstdint>

using HardwarePrintCallback = void (*)(const uint8_t* data, std::size_t length);

class Logger {
  private:
    inline static HardwarePrintCallback s_print_callback = nullptr;

  public:
    Logger() = delete;

    static void init(HardwarePrintCallback callback) {
        s_print_callback = callback;
    }

    template <typename... Args> static void print(const char* fmt, Args... args) {
        if (!s_print_callback) {
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
            s_print_callback(reinterpret_cast<const uint8_t*>(buffer.data()), len);
        }
    }

    template <typename... Args> static void println(const char* fmt, Args... args) {
        print(fmt, args...);

        if (s_print_callback) {
            const std::array<uint8_t, 2> crlf = {'\r', '\n'};
            s_print_callback(crlf.data(), crlf.size());
        }
    }
};

#else

class Logger {
  public:
    Logger() = delete;

    template <typename... Args> static void print(const char* fmt, Args... args) {
        if constexpr (sizeof...(Args) == 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
            std::printf("%s", fmt);
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
            std::printf(fmt, args...);
        }
    }

    template <typename... Args> static void println(const char* fmt, Args... args) {
        print(fmt, args...);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        std::printf("\n");
    }
};

#endif
