#include "device_base.hpp"
#include "errors.hpp"

namespace Sensor {

    void DeviceBase::set_wait_ms(WaitFn fn) noexcept {
        wait_ms_fn = fn;
    }

    bool DeviceBase::wait_ready() noexcept {
        return wait_ms_fn != nullptr;
    }

    bool DeviceBase::wait_ms(uint32_t ms) noexcept {
        if (ms == 0U) {
            return true;
        }
        if (!wait_ready()) {
            return false;
        }

        wait_ms_fn(ms);
        return true;
    }

    bool DeviceBase::is_failed() const noexcept {
        return failed_;
    }

    void DeviceBase::disable(const Error& e) noexcept {
        last_err_ = e;

        failed_ = true;
        arm_retry();
    }

    const Error& DeviceBase::last_error() const noexcept {
        return last_err_;
    }

    void DeviceBase::register_failure(const Error& e) noexcept {
        last_err_ = e;
        if (++fail_count_ >= MAX_FAILURES) {
            failed_ = true;
            arm_retry();
        }
    }

    void DeviceBase::arm_retry() noexcept {
        failed_at_cyc_ = (ErrorClock::now_cycles != nullptr) ? ErrorClock::now_cycles() : 0U;
    }

    bool DeviceBase::retry_due() const noexcept {
        if (!failed_ || ErrorClock::now_cycles == nullptr) {
            return false;
        }

        // cycle domain: the us clock wraps every 53.7 s (< 2x the cooldown), so
        // us differences would fire early on more than half of all cooldowns
        return ErrorClock::us_between(failed_at_cyc_, ErrorClock::now_cycles()) >= RETRY_COOLDOWN_US;
    }

    void DeviceBase::clear_failures() noexcept {
        fail_count_ = 0U;
    }

    void DeviceBase::clear_latch() noexcept {
        failed_ = false;
        fail_count_ = 0U;
    }

} // namespace Sensor
