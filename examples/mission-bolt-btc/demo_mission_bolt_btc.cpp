#include <print>
#include <utility>

#include <samples.hpp>
#include <tmp117.hpp>

#ifdef BUILD_TARGET_HARDWARE
#include <cmsis_i2c_bus.hpp>
// NOLINTNEXTLINE(readability-identifier-naming)
extern ARM_DRIVER_I2C Driver_I2C1;
#else
#include <mock_i2c_bus.hpp>
#endif

void print_error(const Sensor::Error& e) {
    std::println("===== Error =====");
    std::println("  Code:      {}", std::to_underlying(e.code));
    std::println("  Line:      {}", e.line);
    std::println("  Timestamp: {} us", e.timestamp_us);

    std::print("  Trace:     [");
    for (uint8_t i = 0U; i < e.depth; ++i) {
        std::print("0x{:02X}{}", std::to_underlying(e.trace[i]), (i < e.depth - 1U) ? " <- " : "");
    }
    std::println("]{}", e.truncated ? " (TRUNCATED)" : "");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    std::println("  Context:   0x{:08X}", e.context.raw);
}

int main() {
#ifdef BUILD_TARGET_HARDWARE
    Sensor::CmsisI2CBus bus(&Driver_I2C1);
#else
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

    const auto temp_sample = tmp117.read();
    if (!temp_sample) {
        print_error(temp_sample.error());
        return -1;
    }

    std::println("Raw Value: {}", temp_sample->raw_value);

    return 0;
}
