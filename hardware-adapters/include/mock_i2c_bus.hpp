#pragma once

#include <initializer_list>
#include <map>

#include "i2c_bus.hpp"

namespace Sensor {

    /// Mock I2C adapter for CPU-side unit testing and running example programs off-target.
    ///
    /// The bus is populated by \ref init() with one or more register-based devices. Reads and writes resolve against
    /// those register maps and the same error contract as the real adapter is produced.
    ///
    /// Asynchronous errors are not mocked. Without a live peripheral there is nothing to time out, lose arbitration or
    /// fault the bus.
    ///
    /// All fallible calls return Result<void> or Result<value>.
    ///
    /// @ingroup adapter
    class MockI2CBus final : public I2CBus {
      public:
        using RegisterMap = std::map<uint8_t, uint16_t>; // register address -> value

        explicit MockI2CBus() noexcept = default;

        /// Must be called once before any read/write. Populates the bus with the given devices.
        [[nodiscard]] Result<void> init(std::initializer_list<std::pair<const uint8_t, RegisterMap>> devices) noexcept;

        /// Selects a specific register manually for a device on the bus.
        [[nodiscard]] Result<void> select_register(uint8_t addr, uint8_t reg) noexcept;

        /// Write one or two bytes to the register named by data[0].
        [[nodiscard]] Result<void> write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept override;

        /// Read one or two bytes from the currently selected register.
        [[nodiscard]] Result<void> read(uint8_t addr, uint8_t* data, std::size_t len) noexcept override;

        /// Select the register named by tx[0] and read one or two bytes from it.
        [[nodiscard]] Result<void> write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                              std::size_t rx_len) noexcept override;

        [[nodiscard]] Result<void> write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept override;
        [[nodiscard]] Result<void> write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept override;
        [[nodiscard]] Result<uint8_t> read_reg8(uint8_t addr, uint8_t reg) noexcept override;
        [[nodiscard]] Result<uint16_t> read_reg16(uint8_t addr, uint8_t reg) noexcept override;

      private:
        /// Per-device simulation state. Contains the register map and the register currently selected for a pending
        /// read (the I2C slave's register pointer).
        struct DeviceState {
            RegisterMap registers;          ///< register address -> stored value
            uint8_t selected_register = 0U; ///< register latched by the last write_read
        };

        std::map<uint8_t, DeviceState> devices_; ///< device address -> state; empty until init()
    };

} // namespace Sensor
