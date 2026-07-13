#pragma once

#include <cstddef>
#include <cstdint>

#include "Driver_I2C.h"

#include "errors.hpp"
#include "i2c_bus.hpp"

namespace Sensor {

    /// @defgroup adapter Hardware adapters

    /// Wraps ARM_DRIVER_I2C with a register-oriented API. One instance per
    /// physical I2C controller. Every transaction is bounded: by the
    /// optional ms-tick when supplied, otherwise by a fixed-iteration spin
    /// (I-2). All fallible calls return Result<void> or Result<value>.
    ///
    /// @ingroup adapter
    class CmsisI2CBus final : public I2CBus {
      public:
        using TickFn = uint32_t (*)();

        /// Core constructor requiring an explicit tick function for precise hardware timeouts.
        explicit CmsisI2CBus(ARM_DRIVER_I2C* drv, TickFn tick) noexcept
            : drv_(drv),
              get_tick_(tick) {
        }

        /// Falls back to iteration-count timeouts.
        explicit CmsisI2CBus(ARM_DRIVER_I2C* drv) noexcept
            : CmsisI2CBus(drv, nullptr) {
        }

        /// Initialize the underlying CMSIS driver. Must be called once before
        /// any read/write.
        [[nodiscard]] Result<void> init() noexcept;

        /// Full peripheral reset: powers the CMSIS driver off (HAL_I2C_DeInit,
        /// which releases SCL/SDA and clears any latched BERR / arbitration-lost /
        /// lock-up), then re-runs init().
        [[nodiscard]] Result<void> reset() noexcept;

        [[nodiscard]] Result<void> write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept override;
        [[nodiscard]] Result<void> read(uint8_t addr, uint8_t* data, std::size_t len) noexcept override;

        /// Write then read with repeated start.
        [[nodiscard]] Result<void> write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                              std::size_t rx_len) noexcept override;

        [[nodiscard]] Result<void> write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept override;
        [[nodiscard]] Result<void> write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept override;
        [[nodiscard]] Result<uint8_t> read_reg8(uint8_t addr, uint8_t reg) noexcept override;
        [[nodiscard]] Result<uint16_t> read_reg16(uint8_t addr, uint8_t reg) noexcept override;

      private:
        /// SignalEvent callback registered with the CMSIS driver. The callback
        /// type carries no user context, so the latched event is necessarily
        /// shared static state
        static void signal_event(uint32_t event) noexcept;
        static volatile uint32_t last_event;

        /// Block until the SignalEvent callback reports the transaction finished,
        /// then map the event bitmask to a Result.
        [[nodiscard]] Result<void> wait_complete() noexcept;

        [[nodiscard]] bool wait_idle() noexcept;

      private:
        static constexpr uint32_t BUSY_TIMEOUT = 100000U;
        static constexpr uint32_t I2C_TIMEOUT_MS = 25U;

        ARM_DRIVER_I2C* drv_ = nullptr;
        TickFn get_tick_ = nullptr;
    };

} // namespace Sensor
