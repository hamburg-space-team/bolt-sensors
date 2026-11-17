#include <array>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <utility>

#include "delay.hpp"
#include "device_base.hpp"
#include "errors.hpp"
#include "logger.hpp"

#include "icm42686.hpp"
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

#include "mock_i2c_bus.hpp"
#include "mock_sensors.hpp"

#endif

void print_error(const Sensor::Error& e, const char* component_name) {
    Logger::println("===== %s Error =====\x1B[K", component_name);
    Logger::println("  Code:      %u\x1B[K", std::to_underlying(e.code));
    Logger::println("  Line:      %u\x1B[K", e.line);
    Logger::println("  Timestamp: %" PRIu32 " us\x1B[K", e.timestamp_us);

    Logger::print("  Trace:     [");
    for (uint8_t i = 0U; i < e.depth; ++i) {
        Logger::print("0x%02X%s", std::to_underlying(e.trace[i]), (i < e.depth - 1U) ? " <- " : "");
    }
    Logger::println("]%s\x1B[K", e.truncated ? " (TRUNCATED)" : "");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    Logger::println("  Context:   0x%08" PRIX32 "\x1B[K", e.context.raw);
}

void float_to_string(float value, std::span<char> out) {
    if (out.empty()) {
        return;
    }

    const char* sign = (value < 0.0F) ? "-" : "+";

    const auto scaled = static_cast<uint32_t>(std::lroundf(std::abs(value) * 100.0F));
    const uint32_t whole = scaled / 100U;
    const uint32_t frac = scaled % 100U;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    std::snprintf(out.data(), out.size(), "%s%u.%02u", sign, static_cast<unsigned>(whole), static_cast<unsigned>(frac));
}

#ifdef BUILD_TARGET_HARDWARE
extern "C" int app_main(void) {
    Logger::init(board_uart_transmit);

    // Enable DWT cycle counter.
    CoreDebug->DHCSR |= CoreDebug_DHCSR_C_DEBUGEN_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    Sensor::ErrorClock::now_us = []() -> uint32_t { return DWT->CYCCNT / (SystemCoreClock / 1000000U); };
    Sensor::ErrorClock::now_cycles = []() -> uint32_t { return DWT->CYCCNT; };
    Sensor::ErrorClock::cycles_per_us = SystemCoreClock / 1000000U;

    Sensor::CmsisI2CBus bus(&Driver_I2C1);

    if (const auto r = bus.init(); !r) {
        print_error(r.error(), "CmsisI2CBus");
        return -1;
    }
#else
int main() {
    static const auto program_start = std::chrono::steady_clock::now();

    Sensor::ErrorClock::now_us = []() -> uint32_t {
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - program_start)
                .count());
    };
    Sensor::ErrorClock::now_cycles = []() -> uint32_t {
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - program_start)
                .count());
    };
    Sensor::ErrorClock::cycles_per_us = 1000U;

    Sensor::MockI2CBus bus;

    if (const auto r = bus.init({{Sensor::TMP117::DEFAULT_ADDR, MockSensors::tmp117_regs()},
                                 {Sensor::ICM42686::DEFAULT_ADDR, MockSensors::icm42686_regs()}});
        !r) {
        print_error(r.error(), "MockI2CBus");
        return -1;
    }

    if (const auto r = bus.init_memory({{Sensor::ICM42686::DEFAULT_ADDR, MockSensors::icm42686_memory()}}); !r) {
        print_error(r.error(), "MockI2CBus");
        return -1;
    }
#endif
    Sensor::DeviceBase::set_wait_ms(delay);

    Sensor::TMP117 tmp117(bus);
    if (const auto r = tmp117.init(); !r) {
        print_error(r.error(), "TMP117");
        return -1;
    }

    Sensor::ICM42686 icm42686(bus);
    if (const auto r = icm42686.init(); !r) {
        print_error(r.error(), "ICM42686");
        return -1;
    }

    Logger::print("\x1B[2J"); // erase the entire serial screen

    while (true) {
        Logger::print("\x1B[H"); // move cursor to the start

        // TMP117
        {
            const auto temp_sample = tmp117.read();

            if (!temp_sample) {
                print_error(temp_sample.error(), "TMP117");
            } else {
                const float temp_c = temp_sample->celsius();
                const float temp_f = temp_sample->fahrenheit();
                const float temp_k = temp_sample->kelvin();

                std::array<char, 16> c{}, f{}, k{};
                float_to_string(temp_c, c);
                float_to_string(temp_f, f);
                float_to_string(temp_k, k);

                Logger::println("TMP117\t\tRAW %6d\tC %8s\tF %8s\tK %8s \x1B[K", temp_sample->raw_value, c.data(),
                                f.data(), k.data());
            }
        }

        Logger::println("");

        // ICM42686
        {
            const auto imu_sample = icm42686.read();

            if (!imu_sample) {
                print_error(imu_sample.error(), "ICM42686");
            } else {
                std::array<char, 16> ax{}, ay{}, az{}, accel_mag{};
                float_to_string(imu_sample->accel_g_x(), ax);
                float_to_string(imu_sample->accel_g_y(), ay);
                float_to_string(imu_sample->accel_g_z(), az);
                float_to_string(imu_sample->accel_mag(), accel_mag);

                std::array<char, 16> gx{}, gy{}, gz{}, gyro_mag{};
                float_to_string(imu_sample->gyro_dps_x(), gx);
                float_to_string(imu_sample->gyro_dps_y(), gy);
                float_to_string(imu_sample->gyro_dps_z(), gz);
                float_to_string(imu_sample->gyro_mag(), gyro_mag);

                Logger::println("ICM42686\tAX %+6d\tAY %+6d\tAZ %+6d  |%7s %7s %7s g    |  |A| = %7s g \x1B[K",
                                imu_sample->raw_ax, imu_sample->raw_ay, imu_sample->raw_az, ax.data(), ay.data(),
                                az.data(), accel_mag.data());

                Logger::println("ICM42686\tGX %+6d\tGY %+6d\tGZ %+6d  |%7s %7s %7s dps  |  |W| = %7s dps \x1B[K",
                                imu_sample->raw_gx, imu_sample->raw_gy, imu_sample->raw_gz, gx.data(), gy.data(),
                                gz.data(), gyro_mag.data());
            }
        }

        Logger::print("\x1B[J"); // erase leftover lines

        if (tmp117.is_failed() && tmp117.retry_due()) {
            (void)tmp117.init();
        }
        if (icm42686.is_failed() && icm42686.retry_due()) {
            (void)icm42686.init();
        }

        delay(1000);
    }

    return 0;
}
