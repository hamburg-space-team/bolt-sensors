#include "tmp117.hpp"

namespace Sensor {

    Result<void> TMP117::init() noexcept {
        auto dev_id = this->bus.read_reg16(addr, REG_DEV_ID);
        if (!dev_id) {
            return std::unexpected(dev_id.error());
        }

        if ((*dev_id & DEV_ID_MASK) != DEV_ID_EXPECTED) {
            disable(); // Immediately disable: A device responded but it is not a TMP117.
            return std::unexpected(Error::PROTOCOL_ERROR);
        }

        return this->bus.write_reg16(addr, REG_CONFIG, CONFIG_CONTINUOUS);
    }

    Result<TemperatureSample> TMP117::read() noexcept {
        if (is_failed()) {
            return std::unexpected(Error::DISABLED);
        }

        auto raw_u = this->bus.read_reg16(addr, REG_TEMP);
        if (!raw_u) {
            register_failure();
            return std::unexpected(raw_u.error());
        }

        clear_failures();
        return TemperatureSample{static_cast<int16_t>(*raw_u)};
    }

} // namespace Sensor
