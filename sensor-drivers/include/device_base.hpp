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
        /// Hardware target provided blocking millisecond wait.
        using WaitFn = void (*)(uint32_t) noexcept;

        /// Set the blocking wait service used by sensor drivers to settle delays.
        static void set_wait_ms(WaitFn fn) noexcept;

        /// True if a wait service has been registered.
        [[nodiscard]] static bool wait_ready() noexcept;

        /// True once the driver has been latched as failed.
        [[nodiscard]] bool is_failed() const noexcept;

        /// Force the failed state immediately. Used by init() paths that
        /// detect an unrecoverable problem before any failure counter has
        /// been incremented (WHO_AM_I mismatch, PROM CRC fail, ...).
        void disable(const Error& e) noexcept;

        /// The Error that most recently registered a failure (the one
        /// that latched the device, unless calls kept failing after the
        /// latch). Default-constructed while no failure was ever seen.
        [[nodiscard]] const Error& last_error() const noexcept;

        /// Retry gate for a latched device.
        [[nodiscard]] bool retry_due() const noexcept;
        void arm_retry() noexcept;

      protected:
        static constexpr uint8_t MAX_FAILURES = 3U;
        static constexpr uint32_t RETRY_COOLDOWN_US = 30U * 1000U * 1000U; // 30 s deactivation before a retry

        /// Clear the failed latch after a successful in-place recovery (re-init)
        void clear_latch() noexcept;

        /// Wait using the registered target wait service. Returns false if a
        /// non-zero wait was requested but no wait service has been registered.
        [[nodiscard]] static bool wait_ms(uint32_t ms) noexcept;

        void register_failure(const Error& e) noexcept;
        void clear_failures() noexcept;

      private:
        Error last_err_ = {};
        bool failed_ = false;

        uint8_t fail_count_ = 0U;
        uint32_t failed_at_cyc_ = 0U; // raw cycles at latch; start of the retry cooldown

        static inline WaitFn wait_ms_fn = nullptr;
    };

} // namespace Sensor
