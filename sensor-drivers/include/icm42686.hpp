#pragma once

#include <cstdint>

#include "device_base.hpp"
#include "errors.hpp"
#include "i2c_bus.hpp"

#include "icm42686_sample.hpp"

namespace Sensor {

    /// TDK InvenSense ICM-42686-P, 6-axis IMU. Blocking driver, configures
    /// +-32g accel, +-2000 dps gyro. ODR selectable; optionally routes the
    /// data-ready interrupt to INT1 (pulsed, push-pull, active high).
    ///
    /// @ingroup sensors
    class ICM42686 final : public DeviceBase {
      public:
        using Sample = ICM42686Sample;

        /// Output data rate for accel + gyro (ODR bits of *_CONFIG0).
        enum class DataRate : uint8_t { OUT_1KHZ = 0x06U, OUT_200HZ = 0x07U };

        static constexpr uint8_t DEFAULT_ADDR = 0x69U;     // AP_AD0 = 1
        static constexpr uint8_t DEFAULT_ADDR_ALT = 0x68U; // AP_AD0 = 0

        explicit ICM42686(I2CBus& bus, uint8_t addr, DataRate odr) noexcept
            : bus_(bus),
              addr_(addr),
              odr_bits_(static_cast<uint8_t>(odr)) {
        }

        explicit ICM42686(I2CBus& bus, uint8_t addr) noexcept
            : bus_(bus),
              addr_(addr),
              odr_bits_(static_cast<uint8_t>(DataRate::OUT_1KHZ)) {
        }

        explicit ICM42686(I2CBus& bus) noexcept
            : bus_(bus),
              addr_(DEFAULT_ADDR),
              odr_bits_(static_cast<uint8_t>(DataRate::OUT_1KHZ)) {
        }

        /// Verify WHO_AM_I, configure ranges, power on. Failure latches
        /// the driver via disable(). With drdy_int1 the DRDY signal is
        /// routed to INT1 (BTC: wired to PB11, EXTI rising edge).
        [[nodiscard]] Result<void> init(bool drdy_int1) noexcept;
        [[nodiscard]] Result<void> init() noexcept; // defaults drdy_int1 to false

        /// Burst-read the 12 raw register bytes and return a sample. Failures
        /// latch via DeviceBase.
        [[nodiscard]] Result<Sample> read() noexcept;

        /// Raw WHO_AM_I register (self-test WHOAMI). Diagnostic only - no
        /// failure latch, no cold-boot retry loop
        [[nodiscard]] Result<uint8_t> read_who_am_i() noexcept;

      private:
        I2CBus& bus_;
        uint8_t addr_ = 0U;
        uint8_t odr_bits_ = 0U;

        [[nodiscard]] Result<void> check_who_am_i() noexcept;
        [[nodiscard]] Result<void> config_accel() noexcept;
        [[nodiscard]] Result<void> config_gyro() noexcept;
        [[nodiscard]] Result<void> config_int1() noexcept;
        [[nodiscard]] Result<void> power_on() noexcept;
    };

} // namespace Sensor
