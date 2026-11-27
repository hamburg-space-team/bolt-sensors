#pragma once

#include <cstdint>

namespace Sensor {

    struct MMC5983MARegisters {
        /// --- Registers ---
        static constexpr uint8_t PRODUCT_ID = 0x2FU;

        static constexpr uint8_t MAGN_DATA_XOUT0 = 0x00U;   // Xout[17:10]
        static constexpr uint8_t MAGN_DATA_XOUT1 = 0x01U;   // Xout[9:2]
        static constexpr uint8_t MAGN_DATA_YOUT0 = 0x02U;   // Yout[17:10]
        static constexpr uint8_t MAGN_DATA_YOUT1 = 0x03U;   // Yout[9:2]
        static constexpr uint8_t MAGN_DATA_ZOUT0 = 0x04U;   // Zout[17:10]
        static constexpr uint8_t MAGN_DATA_ZOUT1 = 0x05U;   // Zout[9:2]
        static constexpr uint8_t MAGN_DATA_XYZOUT2 = 0x06U; // X[1:0]|Y[1:0]|Z[1:0]

        static constexpr uint8_t ICR0 = 0x09U; // measurement trigger + SET_RESET mode for zero offset correction
        static constexpr uint8_t ICR1 = 0x0AU; // bandwidth, single-axis inhibit, software-reset
        static constexpr uint8_t ICR2 = 0x0BU; // continuous-mode enable + measurement rate

        static constexpr uint8_t STATUS = 0x08U; // MEAS_M_DONE readiness gate (SET_RESET mode)

        /// --- Values ---
        static constexpr uint8_t PRODUCT_ID_EXPECTED = 0x30U;

        static constexpr uint8_t DATA_BYTES = 7U; // burst-read window XOUT0...XYZOUT2

        // Status register readiness gate
        static constexpr uint8_t MEAS_M_DONE = 0x01U; // bit0: the measurement that was triggered has finished
        static constexpr uint8_t OTP_RD_DONE = 0x10U; // bit4: power-up health check (chip successfully read its memory)

        // ICR0 control register
        static constexpr uint8_t TM_M = 0x01;      // bit0: trigger one magnetic field measurement
        static constexpr uint8_t SR_SET = 0x08U;   // bit3: apply SET coil pulse (magnetize one polarity)
        static constexpr uint8_t SR_RESET = 0x10U; // bit4: apply RESET coil pulse (magnetize opposite)

        // ICR1 control register
        static constexpr uint8_t SW_RST = 0x80U;   // bit7: software reset
        static constexpr uint8_t BW_100HZ = 0x00U; // BW=00, 8ms per measurement, best noise reduction
        static constexpr uint8_t BW_200HZ = 0x01U; // BW=01, 4ms per measurement
        static constexpr uint8_t BW_400HZ = 0x02U; // BW=10, 2ms per measurement
        static constexpr uint8_t BW_800HZ = 0x03U; // BW=11, 0.5ms per measurement

        // ICR2 control register (continuous mode options)
        static constexpr uint8_t CMM_EN = 0x08U;              // bit3: enable continuous measurements
        static constexpr uint8_t CMM_1HZ = CMM_EN | 0x01U;    // CM_Freq=001 (needs BW_100HZ)
        static constexpr uint8_t CMM_10HZ = CMM_EN | 0x02U;   // CM_Freq=010 (needs BW_100HZ)
        static constexpr uint8_t CMM_20HZ = CMM_EN | 0x03U;   // CM_Freq=011 (needs BW_100HZ)
        static constexpr uint8_t CMM_50HZ = CMM_EN | 0x04U;   // CM_Freq=100 (needs BW_100HZ)
        static constexpr uint8_t CMM_100HZ = CMM_EN | 0x05U;  // CM_Freq=101 (needs BW_100HZ)
        static constexpr uint8_t CMM_200HZ = CMM_EN | 0x06U;  // CM_Freq=110 (needs BW_200HZ)
        static constexpr uint8_t CMM_1000HZ = CMM_EN | 0x07U; // CM_Freq=111 (needs BW_800HZ)

        // Timing
        static constexpr uint32_t OTP_READY_TIMEOUT_MS = 20U; // health check timeout after SW_RST (10ms + margin)
        static constexpr uint32_t SR_SETTLE_MS = 1U;          // after a SET/RESET coil pulse before next measurement
        static constexpr uint32_t MEAS_DONE_TIMEOUT_MS = 20U; // poll bound on MEAS_M_DONE (> BW_100HZ conversion time)

        // Cold-power-up grace: retries x delay for the first PRODUCT_ID access.
        static constexpr uint8_t PRODUCT_ID_RETRIES = 5U;
        static constexpr uint32_t PRODUCT_ID_RETRY_DELAY_MS = 2U;

        MMC5983MARegisters() = delete;
    };

} // namespace Sensor
