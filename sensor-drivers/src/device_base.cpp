#include "device_base.hpp"

namespace Sensor {

    bool DeviceBase::is_failed() const noexcept {
        return failed;
    }

    void DeviceBase::disable() noexcept {
        failed = true;
    }

    void DeviceBase::register_failure() noexcept {
        if (++fail_count >= MAX_FAILURES) {
            failed = true;
        }
    }

    void DeviceBase::clear_failures() noexcept {
        fail_count = 0U;
    }

} // namespace Sensor
