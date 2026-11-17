#include "tmp117.hpp"

#include "errors.hpp"
#include "tmp117_registers.hpp"

namespace Sensor {

    Result<void> TMP117::init() noexcept {
        const auto dev_id = bus_.read_reg16(addr_, TMP117Registers::DEV_ID);
        if (!dev_id) {
            const auto e = mark(dev_id.error(), Step::TMP_INIT);
            disable(e.error());
            return e;
        }

        if ((*dev_id & TMP117Registers::DEV_ID_MASK) != TMP117Registers::DEV_ID_EXPECTED) {
            const auto e =
                fail(ErrorCode::PROTOCOL_ERROR, Step::TMP_ID_CHECK, __LINE__, ErrorContext::from_id(*dev_id));
            disable(e.error());
            return e;
        }

        if (const auto r = bus_.write_reg16(addr_, TMP117Registers::CONFIG, TMP117Registers::CONFIG_CONTINUOUS); !r) {
            const auto e = mark(r.error(), Step::TMP_CONFIG);
            disable(e.error());
            return e;
        }

        clear_latch();
        return {};
    }

    Result<TMP117::Sample> TMP117::read() noexcept {
        if (is_failed()) {
            return fail(ErrorCode::DISABLED, Step::TMP_READ, __LINE__, ErrorContext::from_device(addr_));
        }

        const auto raw_u = bus_.read_reg16(addr_, TMP117Registers::TEMP);
        if (!raw_u) {
            const auto marked = mark(raw_u.error(), Step::TMP_READ);
            register_failure(marked.error());

            return marked;
        }

        clear_failures();
        return Sample{static_cast<int16_t>(*raw_u)};
    }

} // namespace Sensor
