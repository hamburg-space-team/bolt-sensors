#include <print>
#include <utility>

#include <mock_i2c_bus.hpp>
#include <samples.hpp>
#include <tmp117.hpp>

int main() {
    Sensor::MockI2CBus bus;
    if (auto result = bus.init(); !result) {
        std::println("I2C initialization failed with error code: {}", std::to_underlying(result.error()));
        return -1;
    }

    Sensor::TMP117 tmp117(bus);
    if (auto result = tmp117.init(); !result) {
        std::println("TMP117 initialization failed with error code: {}", std::to_underlying(result.error()));
        return -1;
    }

    auto temp_sample = tmp117.read();
    if (!temp_sample) {
        std::println("Reading TMP117 temperature sample failed with error code: {}",
                     std::to_underlying(temp_sample.error()));
        return -1;
    }

    std::println("Raw Value: {}", temp_sample->raw_value);

    return 0;
}
