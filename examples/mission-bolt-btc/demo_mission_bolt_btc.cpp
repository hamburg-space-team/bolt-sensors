#include <cinttypes>
#include <utility>

#include "logger.hpp"

#include "samples.hpp"
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
#include <chrono>
#include <thread>

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
        } else {
            Logger::println("Raw Value: %d", temp_sample->raw_value);
        }

#ifdef BUILD_TARGET_HARDWARE
        HAL_Delay(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
    }

    return 0;
}
