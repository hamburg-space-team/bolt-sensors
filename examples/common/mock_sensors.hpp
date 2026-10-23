#pragma once

#include "mock_i2c_bus.hpp"
#include "tmp117_registers.hpp"

class MockSensors {
  public:
    MockSensors() = delete;

    static Sensor::MockI2CBus::RegisterMap tmp117(uint16_t temperature = 0x0C80U) {
        return {{Sensor::TMP117Registers::DEV_ID, Sensor::TMP117Registers::DEV_ID_EXPECTED},
                {Sensor::TMP117Registers::CONFIG, Sensor::TMP117Registers::CONFIG_CONTINUOUS},
                {Sensor::TMP117Registers::TEMP, temperature}};
    }
};
