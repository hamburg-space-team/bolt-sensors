#pragma once

#include <cstdint>

#include "device_base.hpp"
#include "errors.hpp"
#include "i2c_bus.hpp"
#include "samples.hpp"

namespace Sensor {

    /// @defgroup sensors Sensor drivers

    constexpr uint8_t TMP117_DEFAULT_ADDR = 0x48U;

    /// TI TMP117 temperature sensor, continuous conversion mode. After
    /// init() the sensor delivers a fresh sample every ~1s at 64-cycle
    /// averaging. Raw register values are forwarded to ground; 1 LSB =
    /// 1/128 degC, 16-bit signed.
    ///
    /// @ingroup sensors
    class TMP117 final : public DeviceBase {
      public:
        explicit TMP117(I2CBus& bus, uint8_t addr = TMP117_DEFAULT_ADDR) noexcept
            : bus(bus),
              addr(addr) {
        }

        /// Verify device ID and switch to continuous conversion.
        [[nodiscard]] Result<void> init() noexcept;

        /// Read the raw temperature register and return a sample.
        // Failures latch via DeviceBase.
        [[nodiscard]] Result<TemperatureSample> read() noexcept;

      private:
        static constexpr uint8_t REG_TEMP = 0x00U;
        static constexpr uint8_t REG_CONFIG = 0x01U;
        static constexpr uint8_t REG_DEV_ID = 0x0FU;

        static constexpr uint16_t DEV_ID_MASK = 0x0FFFU;
        static constexpr uint16_t DEV_ID_EXPECTED = 0x0117U;

        static constexpr uint16_t CONFIG_CONTINUOUS = 0x0000U;

        I2CBus& bus;
        uint8_t addr = 0U;
    };

} // namespace Sensor
