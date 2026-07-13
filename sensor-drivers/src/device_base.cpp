#include "device_base.hpp"

namespace Sensor {

    bool DeviceBase::is_failed() const noexcept {
        return failed_;
    }

    void DeviceBase::disable() noexcept {
        failed_ = true;
    }

    const Error& DeviceBase::last_error() const noexcept {
        return last_err_;
    }

    void DeviceBase::register_failure(const Error& e) noexcept {
        last_err_ = e;
        if (++fail_count_ >= MAX_FAILURES) {
            failed_ = true;
        }
    }

    void DeviceBase::clear_failures() noexcept {
        fail_count_ = 0U;
    }

} // namespace Sensor
