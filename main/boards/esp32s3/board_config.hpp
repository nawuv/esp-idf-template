#pragma once

#include "sdkconfig.h"

namespace device::board {

inline constexpr unsigned i2c_clock_hz = 100000;
inline constexpr int i2c_timeout_ms = 50;
#if CONFIG_ESPKIT_SENSOR_SHT3X
inline constexpr int sda_gpio = CONFIG_ESPKIT_I2C_SDA_GPIO;
inline constexpr int scl_gpio = CONFIG_ESPKIT_I2C_SCL_GPIO;
inline constexpr unsigned sensor_address = CONFIG_ESPKIT_SHT3X_ADDRESS;
#endif

} // namespace device::board
