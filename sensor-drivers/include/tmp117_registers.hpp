#pragma once

#include <cstdint>

namespace Sensor {

    struct TMP117Registers {
        static constexpr uint8_t TEMP = 0x00U;
        static constexpr uint8_t CONFIG = 0x01U;
        static constexpr uint8_t DEV_ID = 0x0FU;
        static constexpr uint16_t DEV_ID_EXPECTED = 0x0117U;
        static constexpr uint16_t DEV_ID_MASK = 0x0FFFU;
        static constexpr uint16_t CONFIG_CONTINUOUS = 0x0000U;

        TMP117Registers() = delete;
    };

} // namespace Sensor