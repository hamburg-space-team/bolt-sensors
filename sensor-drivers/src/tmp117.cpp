#include "tmp117.hpp"
#include "errors.hpp"

namespace Sensor {

    Result<void> TMP117::init() noexcept {
        const auto dev_id = bus_.read_reg16(addr_, REG_DEV_ID);
        if (!dev_id) {
            return mark(dev_id.error(), Step::TMP_INIT);
        }

        if ((*dev_id & DEV_ID_MASK) != DEV_ID_EXPECTED) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::TMP_ID_CHECK, __LINE__, ErrorContext::from_id(*dev_id));
        }

        if (const auto r = bus_.write_reg16(addr_, REG_CONFIG, CONFIG_CONTINUOUS); !r) {
            return mark(r.error(), Step::TMP_CONFIG);
        }

        return {};
    }

    Result<TemperatureSample> TMP117::read() noexcept {
        if (is_failed()) {
            return fail(ErrorCode::DISABLED, Step::TMP_READ, __LINE__, ErrorContext::from_device(addr_));
        }

        const auto raw_u = bus_.read_reg16(addr_, REG_TEMP);
        if (!raw_u) {
            const auto marked = mark(raw_u.error(), Step::TMP_READ);
            register_failure(marked.error());

            return marked;
        }

        clear_failures();
        return TemperatureSample{static_cast<int16_t>(*raw_u)};
    }

} // namespace Sensor
