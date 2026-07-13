#include "cmsis_i2c_bus.hpp"
#include "errors.hpp"

#include <array>
#include <expected>

namespace Sensor {

    volatile uint32_t CmsisI2CBus::last_event = 0U;

    void CmsisI2CBus::signal_event(uint32_t event) noexcept {
        last_event = event;
    }

    Result<void> CmsisI2CBus::init() noexcept {
        if (drv_ == nullptr) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_INIT, __LINE__);
        }
        if (drv_->Initialize(&CmsisI2CBus::signal_event) != ARM_DRIVER_OK) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_INIT, __LINE__);
        }
        if (drv_->PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_INIT, __LINE__);
        }

        return {};
    }

    Result<void> CmsisI2CBus::reset() noexcept {
        if (drv_ == nullptr) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_RESET, __LINE__);
        }

        // Power off (HAL_I2C_DeInit: aborts any in-flight transfer, releases the
        // pins, clears the peripheral's latched error state) then fully re-init.
        (void)drv_->PowerControl(ARM_POWER_OFF);
        (void)drv_->Uninitialize();

        if (const auto r = init(); !r) {
            return mark(r.error(), Step::I2C_RESET);
        }

        return {};
    }

    Result<void> CmsisI2CBus::write(uint8_t addr, const uint8_t* data, std::size_t len) noexcept {
        if (drv_ == nullptr) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE, __LINE__);
        }

        if (!wait_idle()) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_device(addr));
        }

        last_event = 0U;
        if (drv_->MasterTransmit(addr, data, len, false) != ARM_DRIVER_OK) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_WRITE, __LINE__, ErrorContext::from_device(addr));
        }

        if (const auto r = wait_complete(); !r) {
            return mark(r.error(), Step::I2C_WRITE);
        }

        return {};
    }

    Result<void> CmsisI2CBus::read(uint8_t addr, uint8_t* data, std::size_t len) noexcept {
        if (drv_ == nullptr) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_READ, __LINE__);
        }

        if (!wait_idle()) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_device(addr));
        }

        last_event = 0U;
        if (drv_->MasterReceive(addr, data, len, false) != ARM_DRIVER_OK) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_READ, __LINE__, ErrorContext::from_device(addr));
        }

        if (const auto r = wait_complete(); !r) {
            return mark(r.error(), Step::I2C_READ);
        }

        return {};
    }

    Result<void> CmsisI2CBus::write_read(uint8_t addr, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                                         std::size_t rx_len) noexcept {
        if (drv_ == nullptr) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::I2C_WRITE_READ, __LINE__);
        }

        if (!wait_idle()) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_device(addr));
        }

        last_event = 0U;
        if (drv_->MasterTransmit(addr, tx, tx_len, true) != ARM_DRIVER_OK) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_WRITE_READ, __LINE__, ErrorContext::from_device(addr));
        }

        if (const auto r = wait_complete(); !r) {
            return mark(r.error(), Step::I2C_WRITE_READ);
        }

        // RX phase: read() identifies itself in the trace.
        return read(addr, rx, rx_len);
    }

    // write_reg8/16 and read_reg8/16 are transparent pass-throughs: they add
    // no semantic level of their own, so they forward errors unmarked (ADR-012).
    Result<void> CmsisI2CBus::write_reg8(uint8_t addr, uint8_t reg, uint8_t value) noexcept {
        const std::array<uint8_t, 2> buf = {reg, value};
        return write(addr, buf.data(), buf.size());
    }

    Result<void> CmsisI2CBus::write_reg16(uint8_t addr, uint8_t reg, uint16_t value) noexcept {
        const std::array<uint8_t, 3> buf = {reg, static_cast<uint8_t>(value >> 8U),
                                            static_cast<uint8_t>(value & 0xFFU)};
        return write(addr, buf.data(), buf.size());
    }

    Result<uint8_t> CmsisI2CBus::read_reg8(uint8_t addr, uint8_t reg) noexcept {
        uint8_t buf = 0U;
        if (const auto r = write_read(addr, &reg, 1U, &buf, 1U); !r) {
            return std::unexpected(r.error());
        }

        return buf;
    }

    Result<uint16_t> CmsisI2CBus::read_reg16(uint8_t addr, uint8_t reg) noexcept {
        std::array<uint8_t, 2> buf = {};
        if (const auto r = write_read(addr, &reg, 1U, buf.data(), buf.size()); !r) {
            return std::unexpected(r.error());
        }

        return static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8U) | static_cast<uint16_t>(buf[1]));
    }

    bool CmsisI2CBus::wait_idle() noexcept {
        if (!drv_->GetStatus().busy) {
            return true;
        }

        // Leftover transfer still running (late clock-stretched completion).
        // Give it one timeout period to finish cleanly, then abort it.
        if (get_tick_ != nullptr) {
            const uint32_t start = get_tick_();
            while (drv_->GetStatus().busy) {
                if ((get_tick_() - start) >= I2C_TIMEOUT_MS) {
                    (void)drv_->Control(ARM_I2C_ABORT_TRANSFER, 0U);
                    return !drv_->GetStatus().busy;
                }
            }
        } else {
            uint32_t i = 0U;
            while (drv_->GetStatus().busy) {
                if (++i >= BUSY_TIMEOUT) {
                    (void)drv_->Control(ARM_I2C_ABORT_TRANSFER, 0U);
                    return !drv_->GetStatus().busy;
                }
            }
        }

        return true;
    }

    Result<void> CmsisI2CBus::wait_complete() noexcept {
        if (get_tick_ != nullptr) {
            const uint32_t start = get_tick_();
            while ((last_event & ARM_I2C_EVENT_TRANSFER_DONE) == 0U) {
                if ((get_tick_() - start) >= I2C_TIMEOUT_MS) {
                    // Abort the in-flight transfer: without this the CMSIS
                    // driver stays busy and every later transaction fails too.
                    (void)drv_->Control(ARM_I2C_ABORT_TRANSFER, 0U);
                    return fail(ErrorCode::TIMEOUT, Step::I2C_WAIT_COMPLETE, __LINE__);
                }
            }
        } else {
            uint32_t i = 0U;
            while ((last_event & ARM_I2C_EVENT_TRANSFER_DONE) == 0U) {
                if (++i >= BUSY_TIMEOUT) {
                    (void)drv_->Control(ARM_I2C_ABORT_TRANSFER, 0U);
                    return fail(ErrorCode::TIMEOUT, Step::I2C_WAIT_COMPLETE, __LINE__);
                }
            }
        }

        const uint32_t event = last_event;
        if ((event & (ARM_I2C_EVENT_ADDRESS_NACK | ARM_I2C_EVENT_TRANSFER_INCOMPLETE)) != 0U) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::I2C_WAIT_COMPLETE, __LINE__,
                        ErrorContext::from_bus_state(event));
        }
        if ((event & (ARM_I2C_EVENT_BUS_ERROR | ARM_I2C_EVENT_ARBITRATION_LOST)) != 0U) {
            return fail(ErrorCode::BUS_ERROR, Step::I2C_WAIT_COMPLETE, __LINE__, ErrorContext::from_bus_state(event));
        }

        return {};
    }

} // namespace Sensor
