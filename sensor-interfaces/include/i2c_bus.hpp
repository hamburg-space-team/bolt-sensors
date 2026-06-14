#pragma once

#include <cstdint>

#include "errors.hpp"

namespace Sensor {

    /// Pure virtual interface for I2C communication.
    /// This acts as the boundary between hardware-agnostic sensor drivers
    /// and platform-specific hardware implementations.
    class I2CBus {
      public:
        virtual ~I2CBus() = default;

        [[nodiscard]] virtual Result<void> write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept = 0;
        [[nodiscard]] virtual Result<void> read(uint8_t addr, uint8_t* data, std::size_t len) noexcept = 0;

        /// Write then read with repeated start.
        [[nodiscard]] virtual Result<void> write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                                      std::size_t rx_len) noexcept = 0;

        [[nodiscard]] virtual Result<void> write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept = 0;
        [[nodiscard]] virtual Result<void> write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept = 0;
        [[nodiscard]] virtual Result<uint8_t> read_reg8(uint8_t addr, uint8_t reg) noexcept = 0;
        [[nodiscard]] virtual Result<uint16_t> read_reg16(uint8_t addr, uint8_t reg) noexcept = 0;
    };

} // namespace Sensor
