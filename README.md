# Sensors

**_In active development!_**

## Use

Add the repository as a pack and pick components in the `.cproject.yml`:

```yaml
# .csolution.yml
packs:
  - pack: HST::bolt-sensors
    path: <path to this repository>
```

```yaml
# .cproject.yml
components:
  - component: Sensor:Interfaces
  - component: Sensor:Drivers:Core
  - component: Sensor:Drivers:TMP117
  - component: Sensor:Drivers:ICM42686
  - component: Sensor:Adapters:CMSIS I2C
  # the I2C driver underneath, one of:
  - component: Keil::CMSIS Driver:I2C     # CubeMX I2C on HAL (STM32L4 boards)
  - component: CMSIS Driver:I2C:STM32 LL  # CubeMX I2C on LL (STM32U0, STM32L4)
```

The application code does not depend on that choice:

```cpp
#include "cmsis_i2c_bus.hpp"
#include "tmp117.hpp"

extern ARM_DRIVER_I2C Driver_I2C1; // or #include "stm32_ll_i2c.hpp"

Sensor::DeviceBase::set_wait_ms(delay_ms);
Sensor::CmsisI2CBus bus(&Driver_I2C1, tick_ms);
(void)bus.init();
Sensor::TMP117 tmp117(bus);
(void)tmp117.init();
```

The LL driver needs I2C set to LL in CubeMX, with `MX_I2Cn_Init()` called from
`main.c`. It creates `Driver_I2Cn` for each instance in `MX_Device.h`, polls
every transfer and bounds each one by `Sensor::ErrorClock::now_us`, or by a
loop count when that clock is not set. Its header lists what it does not
support.

## Examples

`examples/examples.csolution.yml`, built with CMSIS-Toolbox:

| Context | Board | I2C driver |
|---|---|---|
| `example.Debug+mission-bolt-btc` | BTC, STM32L476 | HAL (Keil) |

The CMake build at the top level runs the BTC example on the host against
`MockI2CBus`.
