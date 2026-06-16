#include "mock_i2c_bus.hpp"

namespace Sensor {

    Result<void> MockI2CBus::init() noexcept {
        return {};
    }

    Result<void> MockI2CBus::write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept {
        return {};
    }

    Result<void> MockI2CBus::read(uint8_t addr, uint8_t* data, std::size_t len) noexcept {
        return {};
    }

    Result<void> MockI2CBus::write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                        std::size_t rx_len) noexcept {
        return {};
    }

    Result<void> MockI2CBus::write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept {
        return {};
    }

    Result<void> MockI2CBus::write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept {
        return {};
    }

    Result<uint8_t> MockI2CBus::read_reg8(uint8_t addr, uint8_t reg) noexcept {
        return {};
    }

    Result<uint16_t> MockI2CBus::read_reg16(uint8_t addr, uint8_t reg) noexcept {
        return {};
    }

} // namespace Sensor