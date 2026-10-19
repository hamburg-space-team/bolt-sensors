#pragma once

#include "mock_i2c_bus.hpp"
#include "tmp117.hpp"

class SensorMock {
  public:
    SensorMock() = delete;

    static Sensor::MockI2CBus::RegisterMap tmp117(uint16_t temperature = 0x0C80U) {
        return {{Sensor::TMP117::REG_DEV_ID, Sensor::TMP117::DEV_ID_EXPECTED},
                {Sensor::TMP117::REG_CONFIG, Sensor::TMP117::CONFIG_CONTINUOUS},
                {Sensor::TMP117::REG_TEMP, temperature}};
    }
};
