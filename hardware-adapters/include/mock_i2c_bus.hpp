#pragma once

#include "i2c_bus.hpp"

namespace Sensor {

    class MockI2CBus final : public I2CBus {
      public:
        explicit MockI2CBus() noexcept = default;

        /// Must be called once before any read/write.
        [[nodiscard]] Result<void> init() noexcept;

        [[nodiscard]] Result<void> write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept override;
        [[nodiscard]] Result<void> read(uint8_t addr, uint8_t* data, std::size_t len) noexcept override;

        /// Write then read with repeated start.
        [[nodiscard]] Result<void> write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                              std::size_t rx_len) noexcept override;

        [[nodiscard]] Result<void> write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept override;
        [[nodiscard]] Result<void> write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept override;
        [[nodiscard]] Result<uint8_t> read_reg8(uint8_t addr, uint8_t reg) noexcept override;
        [[nodiscard]] Result<uint16_t> read_reg16(uint8_t addr, uint8_t reg) noexcept override;
    };

} // namespace Sensor