#include "espkit/hal/i2c.hpp"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace espkit {

EspI2cDevice::~EspI2cDevice() {
    if (device_) {
        i2c_master_bus_rm_device(static_cast<i2c_master_dev_handle_t>(device_));
    }
    if (bus_) {
        i2c_del_master_bus(static_cast<i2c_master_bus_handle_t>(bus_));
    }
}

bool EspI2cDevice::begin(int sda, int scl, unsigned address, unsigned clock_hz,
                         int timeout_ms) noexcept {
    if (bus_ || sda < 0 || scl < 0 || sda == scl || !GPIO_IS_VALID_OUTPUT_GPIO(sda) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(scl) || address < 0x08 || address > 0x77 || clock_hz == 0 ||
        clock_hz > 400000 || timeout_ms <= 0) {
        return false;
    }
    i2c_master_bus_config_t config{};
    config.i2c_port = -1;
    config.sda_io_num = static_cast<gpio_num_t>(sda);
    config.scl_io_num = static_cast<gpio_num_t>(scl);
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    // External pull-ups must be provided by the board/module.
    i2c_master_bus_handle_t bus = nullptr;
    if (i2c_new_master_bus(&config, &bus) != ESP_OK) {
        return false;
    }
    i2c_device_config_t device_config{};
    device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device_config.device_address = address;
    device_config.scl_speed_hz = clock_hz;
    i2c_master_dev_handle_t device = nullptr;
    if (i2c_master_bus_add_device(bus, &device_config, &device) != ESP_OK) {
        i2c_del_master_bus(bus);
        return false;
    }
    bus_ = bus;
    device_ = device;
    timeout_ms_ = timeout_ms;
    return true;
}

bool EspI2cDevice::write(std::span<const std::uint8_t> bytes) noexcept {
    return device_ && i2c_master_transmit(static_cast<i2c_master_dev_handle_t>(device_),
                                          bytes.data(), bytes.size(), timeout_ms_) == ESP_OK;
}

bool EspI2cDevice::read(std::span<std::uint8_t> bytes) noexcept {
    return device_ && i2c_master_receive(static_cast<i2c_master_dev_handle_t>(device_),
                                         bytes.data(), bytes.size(), timeout_ms_) == ESP_OK;
}

void FreeRtosDelay::sleep_for(Milliseconds duration) noexcept {
    if (duration > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration) + 1);
    }
}

} // namespace espkit
