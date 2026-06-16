#pragma once

#include <cstdint>

namespace Sensor {

    struct TemperatureSample {
        int16_t raw_value;

        // TODO: add helper functions to retrieve the temperature in celcius, kelvin and fahrenheit
    };

} // namespace Sensor