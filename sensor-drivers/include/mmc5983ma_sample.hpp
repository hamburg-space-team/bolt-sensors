#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>

namespace Sensor {

    struct MMC5983MASample {
        static constexpr float MAGN_LSB_PER_G = 16384.0F; ///< sensitivity: 131072 / 8G = 16384 LSB/G
        static constexpr float MAGN_GAUSS_TO_UT = 100.0F; ///< 1 Gauss = 100 microTesla

        // 18-bit signed values, range centered
        int32_t raw_mx = 0;
        int32_t raw_my = 0;
        int32_t raw_mz = 0;

        [[nodiscard]] constexpr float magn_g_x() const noexcept {
            return static_cast<float>(raw_mx) / MAGN_LSB_PER_G;
        }

        [[nodiscard]] constexpr float magn_g_y() const noexcept {
            return static_cast<float>(raw_my) / MAGN_LSB_PER_G;
        }

        [[nodiscard]] constexpr float magn_g_z() const noexcept {
            return static_cast<float>(raw_mz) / MAGN_LSB_PER_G;
        }

        [[nodiscard]] constexpr float magn_ut_x() const noexcept {
            return magn_g_x() * MAGN_GAUSS_TO_UT;
        }

        [[nodiscard]] constexpr float magn_ut_y() const noexcept {
            return magn_g_y() * MAGN_GAUSS_TO_UT;
        }

        [[nodiscard]] constexpr float magn_ut_z() const noexcept {
            return magn_g_z() * MAGN_GAUSS_TO_UT;
        }

        [[nodiscard]] constexpr float raw_magn_mag() const noexcept {
            const auto mx = static_cast<float>(raw_mx);
            const auto my = static_cast<float>(raw_my);
            const auto mz = static_cast<float>(raw_mz);

            return std::sqrt(mx * mx + my * my + mz * mz);
        }

        [[nodiscard]] constexpr float magn_g_mag() const noexcept {
            return raw_magn_mag() / MAGN_LSB_PER_G;
        }

        [[nodiscard]] constexpr float magn_ut_mag() const noexcept {
            return magn_g_mag() * MAGN_GAUSS_TO_UT;
        }

        [[nodiscard]] constexpr float magn_heading_deg() const noexcept {
            return std::atan2(static_cast<float>(raw_my), static_cast<float>(raw_mx)) *
                   (180.0F / std::numbers::pi_v<float>);
        }

        [[nodiscard]] constexpr bool operator==(const MMC5983MASample& other) const noexcept {
            return raw_mx == other.raw_mx && raw_my == other.raw_my && raw_mz == other.raw_mz;
        }

        [[nodiscard]] constexpr bool operator!=(const MMC5983MASample& other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] constexpr bool is_zero() const noexcept {
            return raw_mx == 0 && raw_my == 0 && raw_mz == 0;
        }
    };

} // namespace Sensor
