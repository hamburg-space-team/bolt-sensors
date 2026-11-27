#pragma once

#include <cstdint>

#include "mock_i2c_bus.hpp"

#include "icm42686_registers.hpp"
#include "mmc5983ma_registers.hpp"
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

    static Sensor::MockI2CBus::RegisterMap mmc5983ma_regs() {
        using Regs = Sensor::MMC5983MARegisters;
        return {{Regs::PRODUCT_ID, Regs::PRODUCT_ID_EXPECTED},
                {Regs::STATUS, Regs::OTP_RD_DONE | Regs::MEAS_M_DONE}, // init poll + measure poll both pass
                {Regs::ICR0, 0x00U},
                {Regs::ICR1, Regs::BW_100HZ},
                {Regs::ICR2, 0x00U}};
    }

    static Sensor::MockI2CBus::ByteMap mmc5983ma_memory(int32_t mx = 0, int32_t my = 0, int32_t mz = 0) {
        using Regs = Sensor::MMC5983MARegisters;
        constexpr int32_t center = 131072; // 0 G wire code
        const auto x = static_cast<uint32_t>(mx + center);
        const auto y = static_cast<uint32_t>(my + center);
        const auto z = static_cast<uint32_t>(mz + center);

        return {
            {Regs::MAGN_DATA_XOUT0, static_cast<uint8_t>(x >> 10U)},
            {Regs::MAGN_DATA_XOUT1, static_cast<uint8_t>(x >> 2U)},
            {Regs::MAGN_DATA_YOUT0, static_cast<uint8_t>(y >> 10U)},
            {Regs::MAGN_DATA_YOUT1, static_cast<uint8_t>(y >> 2U)},
            {Regs::MAGN_DATA_ZOUT0, static_cast<uint8_t>(z >> 10U)},
            {Regs::MAGN_DATA_ZOUT1, static_cast<uint8_t>(z >> 2U)},
            {Regs::MAGN_DATA_XYZOUT2, static_cast<uint8_t>(((x & 3U) << 6U) | ((y & 3U) << 4U) | ((z & 3U) << 2U))},
        };
    }
};
