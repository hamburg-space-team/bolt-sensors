#pragma once

#include <cstdint>

#include "icm42686_registers.hpp"
#include "mock_i2c_bus.hpp"
#include "tmp117_registers.hpp"

class MockSensors {
  public:
    MockSensors() = delete;

    static Sensor::MockI2CBus::RegisterMap tmp117_regs(uint16_t temperature = 0x0C80U) {
        return {{Sensor::TMP117Registers::DEV_ID, Sensor::TMP117Registers::DEV_ID_EXPECTED},
                {Sensor::TMP117Registers::CONFIG, Sensor::TMP117Registers::CONFIG_CONTINUOUS},
                {Sensor::TMP117Registers::TEMP, temperature}};
    }

    static Sensor::MockI2CBus::RegisterMap icm42686_regs() {
        using Regs = Sensor::ICM42686Registers;

        return {{Regs::WHO_AM_I, Regs::WHO_AM_I_EXPECTED},
                {Regs::DEVICE_CONFIG, 0x00U},
                {Regs::PWR_MGMT0, 0x00U},
                {Regs::ACCEL_CONFIG0, 0x00U},
                {Regs::GYRO_CONFIG0, 0x00U},
                {Regs::INT_CONFIG, 0x00U},
                {Regs::INT_CONFIG1, 0x00U},
                {Regs::INT_SOURCE0, 0x00U}};
    }

    static Sensor::MockI2CBus::ByteMap icm42686_memory(int16_t ax = 0, int16_t ay = 0, int16_t az = 1024,
                                                       int16_t gx = 0, int16_t gy = 0, int16_t gz = 0) {
        using Regs = Sensor::ICM42686Registers;

        return Sensor::MockI2CBus::from_16bit({
            {Regs::ACCEL_DATA_X1 + 0U, static_cast<uint16_t>(ax)},
            {Regs::ACCEL_DATA_X1 + 2U, static_cast<uint16_t>(ay)},
            {Regs::ACCEL_DATA_X1 + 4U, static_cast<uint16_t>(az)},
            {Regs::ACCEL_DATA_X1 + 6U, static_cast<uint16_t>(gx)},
            {Regs::ACCEL_DATA_X1 + 8U, static_cast<uint16_t>(gy)},
            {Regs::ACCEL_DATA_X1 + 10U, static_cast<uint16_t>(gz)},
        });
    }
};
