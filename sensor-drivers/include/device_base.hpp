#pragma once

#include <cstdint>

#include "errors.hpp"

namespace Sensor {

    /// @defgroup core Core

    /// Common fault-management state for sensor drivers.
    ///
    /// Inherit and call register_failure() on a failed bus transaction.
    /// After MAX_FAILURES consecutive failures the driver latches as
    /// failed and further calls should short-circuit. Failures are never
    /// cleared automatically in flight - see ADR-005.
    ///
    /// @ingroup core
    class DeviceBase {
      public:
        /// True once the driver has been latched as failed.
        [[nodiscard]] bool is_failed() const noexcept;

        /// Force the failed state immediately. Used by init() paths that
        /// detect an unrecoverable problem before any failure counter has
        /// been incremented (WHO_AM_I mismatch, PROM CRC fail, ...).
        void disable() noexcept;

        /// The Error that most recently registered a failure (the one
        /// that latched the device, unless calls kept failing after the
        /// latch). Default-constructed while no failure was ever seen.
        [[nodiscard]] const Error& last_error() const noexcept;

      protected:
        static constexpr uint8_t MAX_FAILURES = 3U;

        void register_failure(const Error& e) noexcept;
        void clear_failures() noexcept;

      private:
        Error last_err_ = {};
        bool failed_ = false;
        uint8_t fail_count_ = 0U;
    };

} // namespace Sensor
