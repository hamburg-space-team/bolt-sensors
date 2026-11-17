#pragma once

#include <cstdint>

namespace Sensor {

    struct ICM42686Registers {
        /// --- Registers ---
        static constexpr uint8_t WHO_AM_I = 0x75U;
        static constexpr uint8_t DEVICE_CONFIG = 0x11U;
        static constexpr uint8_t PWR_MGMT0 = 0x4EU;
        static constexpr uint8_t ACCEL_CONFIG0 = 0x50U;
        static constexpr uint8_t GYRO_CONFIG0 = 0x4FU;

        static constexpr uint8_t ACCEL_DATA_X1 = 0x1FU;

        static constexpr uint8_t INT_CONFIG = 0x14U;
        static constexpr uint8_t INT_CONFIG1 = 0x64U;
        static constexpr uint8_t INT_SOURCE0 = 0x65U;

        /// --- Values ---
        static constexpr uint8_t WHO_AM_I_EXPECTED = 0x44U; // expected identity

        static constexpr uint8_t SOFT_RESET = 0x01U; // DEVICE_CONFIG bit0, wait 1 ms after
        static constexpr uint32_t SOFT_RESET_DELAY_MS = 2U;

        // Configured full-scale ranges
        static constexpr uint8_t ACCEL_FS_32G = 0x00U;   // bits 7:5 = 000
        static constexpr uint8_t GYRO_FS_2000 = 0x01U;   // bits 7:5 = 001
        static constexpr uint8_t PWR_ACCEL_GYRO = 0x0FU; // accel+gyro low-noise mode

        // INT1 configuration (data-ready pipeline)
        static constexpr uint8_t INT1_PUSHPULL_ACTIVE_HIGH = 0x03U; // pulsed, push-pull, active high
        static constexpr uint8_t INT_ASYNC_RESET_CLEAR = 0x00U;     // bit4 must be 0 (datasheet 14.5)
        static constexpr uint8_t DRDY_TO_INT1 = 0x08U;              // INT_SOURCE0: UI_DRDY_INT1_EN

        // Cold-power-up grace: retries x delay for the first WHO_AM_I access.
        static constexpr uint8_t WHOAMI_RETRIES = 5U;
        static constexpr uint32_t WHOAMI_RETRY_DELAY_MS = 2U;

        ICM42686Registers() = delete;
    };

} // namespace Sensor
