#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <utility>

#include "delay.hpp"
#include "logger.hpp"

#include "tmp117.hpp"

#ifdef BUILD_TARGET_HARDWARE

#include "cmsis_i2c_bus.hpp"

#include "main.h" // IWYU pragma: keep

extern "C" UART_HandleTypeDef hlpuart1;

// NOLINTNEXTLINE(readability-identifier-naming)
extern ARM_DRIVER_I2C Driver_I2C1;

void board_uart_transmit(const uint8_t* data, std::size_t length) {
    HAL_UART_Transmit(&hlpuart1, data, static_cast<uint16_t>(length), HAL_MAX_DELAY);
}

#else
#include "mock_i2c_bus.hpp"
#endif

void print_error(const Sensor::Error& e) {
    Logger::println("===== Error =====");
    Logger::println("  Code:      %u", std::to_underlying(e.code));
    Logger::println("  Line:      %u", e.line);
    Logger::println("  Timestamp: %" PRIu32 " us", e.timestamp_us);

    Logger::print("  Trace:     [");
    for (uint8_t i = 0U; i < e.depth; ++i) {
        Logger::print("0x%02X%s", std::to_underlying(e.trace[i]), (i < e.depth - 1U) ? " <- " : "");
    }
    Logger::println("]%s", e.truncated ? " (TRUNCATED)" : "");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    Logger::println("  Context:   0x%08" PRIX32, e.context.raw);
}

#ifdef BUILD_TARGET_HARDWARE
extern "C" int app_main(void) {
    Logger::init(board_uart_transmit);

    Sensor::CmsisI2CBus bus(&Driver_I2C1);
#else
int main() {
    Sensor::MockI2CBus bus;
#endif

    if (const auto r = bus.init(); !r) {
        print_error(r.error());
        return -1;
    }

    Sensor::TMP117 tmp117(bus);
    if (const auto r = tmp117.init(); !r) {
        print_error(r.error());
        return -1;
    }

    while (true) {
        const auto temp_sample = tmp117.read();

        if (!temp_sample) {
            print_error(temp_sample.error());
            delay(1000);
            continue;
        }

        const float temp_c = temp_sample->celsius();
        const float temp_f = temp_sample->fahrenheit();
        const float temp_k = temp_sample->kelvin();

        const char* temp_c_sign = (temp_c < 0.0F) ? "-" : "";
        const auto temp_c_whole = static_cast<uint16_t>(temp_c);
        const auto temp_c_frac = static_cast<uint8_t>(std::abs(temp_c - static_cast<float>(temp_c_whole)) * 100.0F);

        const char* temp_f_sign = (temp_f < 0.0F) ? "-" : "";
        const auto temp_f_whole = static_cast<int16_t>(temp_f);
        const auto temp_f_frac = static_cast<uint8_t>(std::abs(temp_f - static_cast<float>(temp_f_whole)) * 100.0F);

        const char* temp_k_sign = (temp_k < 0.0F) ? "-" : "";
        const auto temp_k_whole = static_cast<uint16_t>(temp_k);
        const auto temp_k_frac = static_cast<uint8_t>(std::abs(temp_k - static_cast<float>(temp_k_whole)) * 100.0F);

        Logger::println("Raw Value: %d; "
                        "Celsius: %s%u.%02u C; "
                        "Fahrenheit: %s%u.%02u F; "
                        "Kelvin: %s%u.%02u K",
                        temp_sample->raw_value, temp_c_sign, temp_c_whole, temp_c_frac, temp_f_sign, temp_f_whole,
                        temp_f_frac, temp_k_sign, temp_k_whole, temp_k_frac);

        delay(1000);
    }

    return 0;
}
