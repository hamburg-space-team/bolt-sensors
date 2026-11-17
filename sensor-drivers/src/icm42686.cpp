#include "icm42686.hpp"

#include <array>

#include "errors.hpp"
#include "icm42686_registers.hpp"

namespace Sensor {

    Result<void> ICM42686::init(bool drdy_int1) noexcept {
        (void)bus_.write_reg8(addr_, ICM42686Registers::DEVICE_CONFIG, ICM42686Registers::SOFT_RESET);

        if (!wait_ms(ICM42686Registers::SOFT_RESET_DELAY_MS)) {
            const auto e = fail(ErrorCode::BAD_ARGUMENT, Step::IMU_INIT, __LINE__);
            disable(e.error());
            return e;
        }

        auto who = check_who_am_i();
        if (!who) {
            addr_ = (addr_ == DEFAULT_ADDR) ? DEFAULT_ADDR_ALT : DEFAULT_ADDR;
            (void)bus_.write_reg8(addr_, ICM42686Registers::DEVICE_CONFIG, ICM42686Registers::SOFT_RESET);

            if (!wait_ms(ICM42686Registers::SOFT_RESET_DELAY_MS)) {
                const auto e = fail(ErrorCode::BAD_ARGUMENT, Step::IMU_INIT, __LINE__);
                disable(e.error());
                return e;
            }

            who = check_who_am_i();
        }
        if (!who) {
            disable(who.error());
            return who;
        }

        if (const auto r = config_accel(); !r) {
            disable(r.error());
            return r;
        }
        if (const auto r = config_gyro(); !r) {
            disable(r.error());
            return r;
        }

        if (drdy_int1) {
            if (const auto r = config_int1(); !r) {
                disable(r.error());
                return r;
            }
        }

        if (const auto r = power_on(); !r) {
            disable(r.error());
            return r;
        }

        clear_latch();
        return {};
    }

    Result<void> ICM42686::init() noexcept {
        return init(false);
    }

    Result<uint8_t> ICM42686::read_who_am_i() noexcept {
        auto val = bus_.read_reg8(addr_, ICM42686Registers::WHO_AM_I);
        if (!val) {
            return mark(val.error(), Step::IMU_WHOAMI);
        }

        return *val;
    }

    Result<void> ICM42686::check_who_am_i() noexcept {
        auto val = bus_.read_reg8(addr_, ICM42686Registers::WHO_AM_I);

        for (uint8_t i = 0U;
             (!val || *val != ICM42686Registers::WHO_AM_I_EXPECTED) && i < ICM42686Registers::WHOAMI_RETRIES; i++) {
            if (!wait_ms(ICM42686Registers::WHOAMI_RETRY_DELAY_MS)) {
                return fail(ErrorCode::BAD_ARGUMENT, Step::IMU_WHOAMI, __LINE__);
            }

            val = bus_.read_reg8(addr_, ICM42686Registers::WHO_AM_I);
        }

        if (!val) {
            return mark(val.error(), Step::IMU_WHOAMI);
        }
        if (*val != ICM42686Registers::WHO_AM_I_EXPECTED) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::IMU_WHOAMI, __LINE__);
        }

        return {};
    }

    Result<void> ICM42686::config_accel() noexcept {
        const uint8_t config = (ICM42686Registers::ACCEL_FS_32G << 5U) | odr_bits_;

        if (const auto r = bus_.write_reg8(addr_, ICM42686Registers::ACCEL_CONFIG0, config); !r) {
            return mark(r.error(), Step::IMU_CONFIG_ACCEL);
        }

        return {};
    }

    Result<void> ICM42686::config_gyro() noexcept {
        const uint8_t config = (ICM42686Registers::GYRO_FS_2000 << 5U) | odr_bits_;

        if (const auto r = bus_.write_reg8(addr_, ICM42686Registers::GYRO_CONFIG0, config); !r) {
            return mark(r.error(), Step::IMU_CONFIG_GYRO);
        }

        return {};
    }

    Result<void> ICM42686::config_int1() noexcept {
        // INT1: pulsed, push-pull, active high -> BTC PB11 (EXTI, rising edge).
        if (const auto r =
                bus_.write_reg8(addr_, ICM42686Registers::INT_CONFIG, ICM42686Registers::INT1_PUSHPULL_ACTIVE_HIGH);
            !r) {
            return mark(r.error(), Step::IMU_CONFIG_INT);
        }

        // INT_ASYNC_RESET (bit 4) must be cleared from its reset value of 1
        // for proper INT1/INT2 operation (datasheet register description).
        if (const auto r =
                bus_.write_reg8(addr_, ICM42686Registers::INT_CONFIG1, ICM42686Registers::INT_ASYNC_RESET_CLEAR);
            !r) {
            return mark(r.error(), Step::IMU_CONFIG_INT);
        }

        if (const auto r = bus_.write_reg8(addr_, ICM42686Registers::INT_SOURCE0, ICM42686Registers::DRDY_TO_INT1);
            !r) {
            return mark(r.error(), Step::IMU_CONFIG_INT);
        }

        return {};
    }

    Result<void> ICM42686::power_on() noexcept {
        if (auto r = bus_.write_reg8(addr_, ICM42686Registers::PWR_MGMT0, ICM42686Registers::PWR_ACCEL_GYRO); !r) {
            return mark(r.error(), Step::IMU_POWER_ON);
        }

        // Datasheet: no register writes for 200us after PWR_MGMT0.
        if (!wait_ms(1U)) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::IMU_POWER_ON, __LINE__);
        }

        return {};
    }

    Result<ICM42686::Sample> ICM42686::read() noexcept {
        if (is_failed()) {
            return fail(ErrorCode::DISABLED, Step::IMU_READ, __LINE__);
        }

        std::array<uint8_t, 12> buf{};
        uint8_t start_reg = ICM42686Registers::ACCEL_DATA_X1;

        auto r = bus_.write_read(addr_, &start_reg, 1U, buf.data(), buf.size());
        if (!r) {
            const uint8_t alt = (addr_ == DEFAULT_ADDR) ? DEFAULT_ADDR_ALT : DEFAULT_ADDR;
            const auto who = bus_.read_reg8(alt, ICM42686Registers::WHO_AM_I);

            if (who && *who == ICM42686Registers::WHO_AM_I_EXPECTED) {
                addr_ = alt;
                r = bus_.write_read(addr_, &start_reg, 1U, buf.data(), buf.size());
            }
        }

        if (!r) {
            const auto marked = mark(r.error(), Step::IMU_READ);
            register_failure(marked.error());
            return marked;
        }

        clear_failures();

        // Shift the high byte into a uint16_t before combining with the low byte to avoid
        // implementation-defined behaviour when casting values > 0x7FFF to int16_t.
        return Sample{
            .raw_ax = static_cast<int16_t>((static_cast<uint16_t>(buf[0]) << 8U) | buf[1]),
            .raw_ay = static_cast<int16_t>((static_cast<uint16_t>(buf[2]) << 8U) | buf[3]),
            .raw_az = static_cast<int16_t>((static_cast<uint16_t>(buf[4]) << 8U) | buf[5]),
            .raw_gx = static_cast<int16_t>((static_cast<uint16_t>(buf[6]) << 8U) | buf[7]),
            .raw_gy = static_cast<int16_t>((static_cast<uint16_t>(buf[8]) << 8U) | buf[9]),
            .raw_gz = static_cast<int16_t>((static_cast<uint16_t>(buf[10]) << 8U) | buf[11]),
        };
    }

} // namespace Sensor
