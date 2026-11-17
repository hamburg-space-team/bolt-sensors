#pragma once

#include <cmath>
#include <cstdint>

namespace Sensor {

    /// One raw sample (6x 16-bit signed registers). Units depend on the
    /// configured full-scale range; see config_accel() / config_gyro().
    /// Physical helpers assume the ICM-42686 was configured for +-32g accel
    /// and +-2000 dps gyro.
    ///
    /// @ingroup sensors
    struct ICM42686Sample {
        static constexpr float ACCEL_LSB_PER_G = 1024.0F;  ///< accelerometer sensitivity: 32768 / 32 = 1024 LSB/g
        static constexpr float GYRO_LSB_PER_DPS = 16.384F; ///< gyroscope sensitivity: 32768 / 2000 = 16.384 LSB/dps

        int16_t raw_ax = 0;
        int16_t raw_ay = 0;
        int16_t raw_az = 0;

        int16_t raw_gx = 0;
        int16_t raw_gy = 0;
        int16_t raw_gz = 0;

        [[nodiscard]] constexpr float accel_g_x() const noexcept {
            return static_cast<float>(raw_ax) / ACCEL_LSB_PER_G;
        }

        [[nodiscard]] constexpr float accel_g_y() const noexcept {
            return static_cast<float>(raw_ay) / ACCEL_LSB_PER_G;
        }

        [[nodiscard]] constexpr float accel_g_z() const noexcept {
            return static_cast<float>(raw_az) / ACCEL_LSB_PER_G;
        }

        [[nodiscard]] constexpr float raw_accel_mag() const noexcept {
            const auto ax = static_cast<float>(raw_ax);
            const auto ay = static_cast<float>(raw_ay);
            const auto az = static_cast<float>(raw_az);

            return std::sqrt(ax * ax + ay * ay + az * az);
        }

        [[nodiscard]] constexpr float accel_mag() const noexcept {
            return raw_accel_mag() / ACCEL_LSB_PER_G;
        }

        [[nodiscard]] constexpr float gyro_dps_x() const noexcept {
            return static_cast<float>(raw_gx) / GYRO_LSB_PER_DPS;
        }

        [[nodiscard]] constexpr float gyro_dps_y() const noexcept {
            return static_cast<float>(raw_gy) / GYRO_LSB_PER_DPS;
        }

        [[nodiscard]] constexpr float gyro_dps_z() const noexcept {
            return static_cast<float>(raw_gz) / GYRO_LSB_PER_DPS;
        }

        [[nodiscard]] constexpr float raw_gyro_mag() const noexcept {
            const auto gx = static_cast<float>(raw_gx);
            const auto gy = static_cast<float>(raw_gy);
            const auto gz = static_cast<float>(raw_gz);

            return std::sqrt(gx * gx + gy * gy + gz * gz);
        }

        [[nodiscard]] constexpr float gyro_mag() const noexcept {
            return raw_gyro_mag() / GYRO_LSB_PER_DPS;
        }

        [[nodiscard]] constexpr bool operator==(const ICM42686Sample& other) const noexcept {
            return raw_ax == other.raw_ax && raw_ay == other.raw_ay && raw_az == other.raw_az &&
                   raw_gx == other.raw_gx && raw_gy == other.raw_gy && raw_gz == other.raw_gz;
        }

        [[nodiscard]] constexpr bool operator!=(const ICM42686Sample& other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] constexpr bool is_zero() const noexcept {
            return raw_ax == 0 && raw_ay == 0 && raw_az == 0 && raw_gx == 0 && raw_gy == 0 && raw_gz == 0;
        }
    };

} // namespace Sensor
