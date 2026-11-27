#pragma once

#include "Driver_I2C.h"

/// @file
/// CMSIS-Driver I2C for STM32 parts whose I2C runs on the LL drivers
/// (CubeMX: I2C set to LL), e.g. the 32 KiB STM32U031 where the HAL does
/// not fit. It provides the same `Driver_I2Cn` objects as ARM's
/// CMSIS-Driver_STM32, so the application code is the same either way:
///
/// @code
/// Sensor::CmsisI2CBus bus(&Driver_I2C1);
/// @endcode
///
/// One `Driver_I2Cn` exists for each instance CubeMX configured (`MX_I2Cn`
/// in MX_Device.h). Clock, pins and timing come from the generated
/// MX_I2Cn_Init(), which main.c has to call before the bus's init().
///
/// Transfers are polled: MasterTransmit/MasterReceive return once the
/// transfer has ended and signal its event before they do. Each transfer is
/// bounded (I-2): by Sensor::ErrorClock::now_us when the application set
/// it, otherwise by a loop count derived from SystemCoreClock.
///
/// Not supported: slave mode, 10-bit addresses, ARM_I2C_BUS_SPEED (the
/// speed is the CubeMX timing), ARM_I2C_BUS_CLEAR, transfers above 255
/// bytes.
///
/// Families: STM32U0, STM32L4 (on the L4 boards ARM's HAL-based driver is
/// in use).
///
/// @ingroup adapter

// NOLINTBEGIN(readability-identifier-naming): CMSIS driver names
extern "C" ARM_DRIVER_I2C Driver_I2C1;
extern "C" ARM_DRIVER_I2C Driver_I2C2;
extern "C" ARM_DRIVER_I2C Driver_I2C3;
extern "C" ARM_DRIVER_I2C Driver_I2C4;
// NOLINTEND(readability-identifier-naming)
