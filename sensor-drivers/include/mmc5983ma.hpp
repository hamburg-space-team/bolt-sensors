#pragma once

#include <array>
#include <cstdint>

#include "device_base.hpp"
#include "errors.hpp"
#include "i2c_bus.hpp"

#include "mmc5983ma_registers.hpp"
#include "mmc5983ma_sample.hpp"

namespace Sensor {

    /// MEMSIC MMC5983MA, 3-axis, +-8G, 18-bit magnetometer.
    ///
    /// @ingroup sensors
    class MMC5983MA final : public DeviceBase {
      public:
        using Sample = MMC5983MASample;

        /// Per-measurement bandwidth (ICR1 BW-bits) that fixes the conversion time and
        /// sets a cap on the sample rate that can be sustained. Used in both modes.
        enum class DataRate : uint8_t {
            OUT_100HZ = MMC5983MARegisters::BW_100HZ, ///< 100 Hz bandwidth -> 8 ms per measurement, lowest noise
            OUT_200HZ = MMC5983MARegisters::BW_200HZ, ///< 200 Hz bandwidth -> 4 ms per measurement
            OUT_400HZ = MMC5983MARegisters::BW_400HZ, ///< 400 Hz bandwidth -> 2 ms per measurement
            OUT_800HZ = MMC5983MARegisters::BW_800HZ, ///< 800 Hz bandwidth -> 0.5 ms per measurement
        };

        /// Continuous-mode output refresh rate (ICR2 Cm_freq and CMM enabled).
        /// Only used when the mode is set to CONTINUOUS. It is the chip's own autonomous
        /// sampling frequency.
        enum class SampleRate : uint8_t {
            CMM_1HZ = MMC5983MARegisters::CMM_1HZ,       ///< 1 Hz (needs OUT_100HZ data rate)
            CMM_10HZ = MMC5983MARegisters::CMM_10HZ,     ///< 10 Hz (needs OUT_100HZ data rate)
            CMM_20HZ = MMC5983MARegisters::CMM_20HZ,     ///< 20 Hz (needs OUT_100HZ data rate)
            CMM_50HZ = MMC5983MARegisters::CMM_50HZ,     ///< 50 Hz (needs OUT_100HZ data rate)
            CMM_100HZ = MMC5983MARegisters::CMM_100HZ,   ///< 100 Hz (needs OUT_100HZ data rate)
            CMM_200HZ = MMC5983MARegisters::CMM_200HZ,   ///< 200 Hz (needs OUT_200HZ data rate)
            CMM_1000HZ = MMC5983MARegisters::CMM_1000HZ, ///< 1 kHz (needs OUT_800HZ data rate)
        };

        /// Explicit operating mode for sampling.
        enum class Mode : uint8_t {
            CONTINUOUS, ///< chip auto-refreshes at SampleRate; read() = last value without offset correction
            SET_RESET,  ///< read() triggers SET/RESET measurements and returns (M_set - M_reset) / 2 (offset cancelled)
        };

        static constexpr uint8_t DEFAULT_ADDR = 0x30U; ///< unrelated to 0x30 PRODUCT_ID

        explicit MMC5983MA(I2CBus& bus, uint8_t addr, Mode mode, DataRate data_rate, SampleRate sample_rate) noexcept
            : bus_(bus),
              addr_(addr),
              mode_(mode),
              data_rate_(data_rate),
              sample_rate_(sample_rate) {
        }

        explicit MMC5983MA(I2CBus& bus, uint8_t addr, Mode mode, DataRate data_rate) noexcept
            : MMC5983MA(bus, addr, mode, data_rate, SampleRate::CMM_100HZ) {
        }

        explicit MMC5983MA(I2CBus& bus, Mode mode) noexcept
            : MMC5983MA(bus, DEFAULT_ADDR, mode, DataRate::OUT_100HZ, SampleRate::CMM_100HZ) {
        }

        explicit MMC5983MA(I2CBus& bus) noexcept
            : MMC5983MA(bus, DEFAULT_ADDR, Mode::SET_RESET, DataRate::OUT_100HZ, SampleRate::CMM_100HZ) {
        }

        /// Verify PRODUCT_ID, soft reset, settle, configure for selected mode. Failure latches
        /// the driver via disable().
        [[nodiscard]] Result<void> init() noexcept;

        /// CONTINUOUS: Burst-read the last 7-byte sample.
        /// SET_RESET:  Trigger two measurements with opposite polarity and return (M_set - M_reset) / 2
        ///             that cancels out the zero offset.
        [[nodiscard]] Result<Sample> read() noexcept;

        [[nodiscard]] Result<uint8_t> read_product_id() const noexcept;

      private:
        I2CBus& bus_;
        uint8_t addr_;

        Mode mode_;
        DataRate data_rate_;
        SampleRate sample_rate_;

        [[nodiscard]] Result<void> check_product_id() const noexcept;
        [[nodiscard]] Result<void> configure() const noexcept;
        [[nodiscard]] Result<std::array<uint32_t, 3>> read_raw() noexcept;

        enum class Polarity : uint8_t { SET, RESET };
        [[nodiscard]] Result<std::array<uint32_t, 3>> measure_polarity(Polarity polarity) noexcept;
    };

} // namespace Sensor
