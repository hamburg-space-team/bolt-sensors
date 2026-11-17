#include "mock_i2c_bus.hpp"
#include "errors.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <initializer_list>

namespace Sensor {

    Result<void> MockI2CBus::init(std::initializer_list<std::pair<const uint8_t, RegisterMap>> devices) noexcept {
        devices_.clear();

        for (const auto& [addr, reg_map] : devices) {
            devices_.emplace(addr, DeviceState{reg_map, {}, 0U});
        }

        return {};
    }

    Result<void> MockI2CBus::init_memory(std::initializer_list<std::pair<const uint8_t, ByteMap>> devices) noexcept {
        for (const auto& [addr, memory] : devices) {
            DeviceState& state = devices_[addr];
            state.memory = memory;
        }

        return {};
    }

    Result<void> MockI2CBus::select_register(uint8_t addr, uint8_t reg) noexcept {
        const auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::NONE, __LINE__, ErrorContext::from_device(addr));
        }

        if (!device->second.registers.contains(reg) && !device->second.memory.contains(reg)) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::NONE, __LINE__, ErrorContext::from_register(reg));
        }

        device->second.register_pointer = reg;
        return {};
    }

    Result<void> MockI2CBus::write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept {
        if (devices_.empty()) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE, __LINE__);
        }
        if (len < 2U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_device(addr));
        }

        const uint8_t reg = data[0];

        if (len > 3U) {
            if (!device->second.memory.contains(reg)) {
                // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
                // mock bus.
                return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_register(reg));
            }

            // Set register pointer and then store data bytes with auto-increment.
            device->second.register_pointer = reg;

            for (std::size_t i = 1U; i < len; i++) {
                device->second.memory[device->second.register_pointer] = data[i];
                device->second.register_pointer++;
            }

            return {};
        }

        const auto reg_it = device->second.registers.find(reg);
        if (reg_it == device->second.registers.end()) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_register(reg));
        }

        if (len == 3U) {
            reg_it->second = static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8U) | data[2]);
        } else {
            reg_it->second = data[1];
        }

        return {};
    }

    Result<void> MockI2CBus::read(uint8_t addr, uint8_t* data, std::size_t len) noexcept {
        if (devices_.empty()) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_READ, __LINE__);
        }
        if (len < 1U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_READ, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_device(addr));
        }

        if (len > 2U) {
            // Read bytes starting from the register pointer and auto-incrementing after each byte.
            for (std::size_t i = 0U; i < len; i++) {
                const uint8_t reg = device->second.register_pointer;

                const auto reg_it = device->second.memory.find(reg);
                if (reg_it == device->second.memory.end()) {
                    // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using
                    // the mock bus.
                    return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_register(reg));
                }

                data[i] = reg_it->second;
                device->second.register_pointer = reg + 1U;
            }

            return {};
        }

        const uint8_t reg = device->second.register_pointer;
        const auto reg_it = device->second.registers.find(reg);
        if (reg_it == device->second.registers.end()) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using
            // the mock bus.
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
        if (tx_len < 1U || rx_len < 1U) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE_READ, __LINE__);
        }

        auto device = devices_.find(addr);
        if (device == devices_.end()) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_device(addr));
        }

        const uint8_t reg = tx[0];
        if (!device->second.registers.contains(reg) && !device->second.memory.contains(reg)) {
            // NOTE: CmsisI2CBus uses from_device but register context might be better for debugging when using the
            // mock bus.
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_register(reg));
        }

        // Select the starting register for the read (simulated I2C repeated-start behaviour).
        device->second.register_pointer = reg;

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

    MockI2CBus::ByteMap MockI2CBus::from_16bit(std::initializer_list<std::pair<uint8_t, uint16_t>> registers) noexcept {
        ByteMap result;

        for (const auto& [reg, val] : registers) {
            result[reg] = static_cast<uint8_t>(val >> 8U);
            result[reg + 1U] = static_cast<uint8_t>(val & 0xFFU);
        }

        return result;
    }

} // namespace Sensor
