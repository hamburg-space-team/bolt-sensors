#include "mock_i2c_bus.hpp"
#include "errors.hpp"

#include <array>
#include <expected>
#include <initializer_list>

namespace Sensor {

    Result<void> MockI2CBus::init(std::initializer_list<std::pair<const uint8_t, RegisterMap>> devices) noexcept {
        devices_.clear();

        for (const auto& [addr, reg_map] : devices) {
            devices_.emplace(addr, DeviceState{reg_map, 0U});
        }

        return {};
    }

    Result<void> MockI2CBus::select_register(uint8_t addr, uint8_t reg) noexcept {
        const auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::NONE, __LINE__, ErrorContext::from_device(addr));
        }

        if (!device->second.registers.contains(reg)) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::NONE, __LINE__, ErrorContext::from_register(reg));
        }

        device->second.selected_register = reg;
        return {};
    }

    Result<void> MockI2CBus::write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept {
        if (devices_.empty()) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE, __LINE__);
        }
        if (len < 2U || len > 3U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_device(addr));
        }

        const uint8_t reg = data[0];
        auto reg_it = device->second.registers.find(reg);
        if (reg_it == device->second.registers.end()) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_register(reg));
        }

        if (len == 3U) {
            reg_it->second = static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8U) | data[2]);
        } else if (len == 2U) {
            reg_it->second = data[1];
        }

        return {};
    }

    Result<void> MockI2CBus::read(uint8_t addr, uint8_t* data, std::size_t len) noexcept {
        if (devices_.empty()) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_READ, __LINE__);
        }
        if (len < 1U || len > 2U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_READ, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_device(addr));
        }

        // Retrieve register that should have been selected before calling a read.
        const uint8_t reg = device->second.selected_register;

        auto reg_it = device->second.registers.find(reg);
        if (reg_it == device->second.registers.end()) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_register(reg));
        }

        const uint16_t value = reg_it->second;
        if (len == 2U) {
            data[0] = static_cast<uint8_t>(value >> 8U);
            data[1] = static_cast<uint8_t>(value & 0xFFU);
        } else {
            data[0] = static_cast<uint8_t>(value & 0xFFU);
        }

        return {};
    }

    Result<void> MockI2CBus::write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                        std::size_t rx_len) noexcept {
        if (devices_.empty()) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE_READ, __LINE__);
        }
        if (tx_len < 1U || rx_len < 1U || rx_len > 2U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE_READ, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_device(addr));
        }

        const uint8_t reg = tx[0];
        if (!device->second.registers.contains(reg)) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_register(reg));
        }

        // Select the register that should be read from (simulated write without stop condition).
        device->second.selected_register = reg;

        return read(addr, rx, rx_len);
    }

    Result<void> MockI2CBus::write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept {
        const std::array<uint8_t, 2> buf = {reg, value};
        return write(addr, buf.data(), buf.size());
    }

    Result<void> MockI2CBus::write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept {
        const std::array<uint8_t, 3> buf = {reg, static_cast<uint8_t>(value >> 8U),
                                            static_cast<uint8_t>(value & 0xFFU)};
        return write(addr, buf.data(), buf.size());
    }

    Result<uint8_t> MockI2CBus::read_reg8(uint8_t addr, uint8_t reg) noexcept {
        uint8_t buf = 0U;
        if (const auto r = write_read(addr, &reg, 1U, &buf, 1U); !r) {
            return std::unexpected(r.error());
        }

        return buf;
    }

    Result<uint16_t> MockI2CBus::read_reg16(uint8_t addr, uint8_t reg) noexcept {
        std::array<uint8_t, 2> buf = {};
        if (const auto r = write_read(addr, &reg, 1U, buf.data(), buf.size()); !r) {
            return std::unexpected(r.error());
        }

        return static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8U) | static_cast<uint16_t>(buf[1]));
    }

} // namespace Sensor
