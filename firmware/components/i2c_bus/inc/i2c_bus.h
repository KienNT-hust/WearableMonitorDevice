#ifndef _i2c_bus_h__
#define _i2c_bus_h__

#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
typedef void *i2c_bus_handle_t; // opaque handle

typedef struct
{
    i2c_port_t port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t clk_speed_hz;
    uint32_t timeout_ms;
    uint8_t clk_flags;
} i2c_bus_config_t;
/*
 * Create an I2C bus instance
 * @param config: pointer to I2C bus configuration structure
 * @return: handle to the created I2C bus instance, or NULL on failure
 */
i2c_bus_handle_t i2c_bus_create(const i2c_bus_config_t *config);

/**
 * Destroy an I2C bus instance
 * @param handle: handle to the I2C bus instance to destroy
 */
void i2c_bus_destroy(i2c_bus_handle_t handle);

/**
 * Read data from an I2C device
 * @param handle: handle to the I2C bus instance
 * @param dev_addr: 7-bit I2C device address
 * @param reg_addr: register address to read from
 * @param buf: buffer to store the read data
 * @param len: length of data to read
 * @return: ESP_OK on success, or an error code on failure
 */
esp_err_t i2c_bus_read(i2c_bus_handle_t handle,
                       uint8_t dev_addr, uint8_t reg_addr,
                       uint8_t *buf, uint16_t len);

/**
 * Write data to an I2C device
 * @param handle: handle to the I2C bus instance
 * @param dev_addr: 7-bit I2C device address
 * @param reg_addr: register address to write to
 * @param buf: buffer containing the data to write
 * @param len: length of data to write
 * @return: ESP_OK on success, or an error code on failure
 */
esp_err_t i2c_bus_write(i2c_bus_handle_t handle,
                        uint8_t dev_addr, uint8_t reg_addr,
                        const uint8_t *buf, uint16_t len);

/**
 * Scan the I2C bus for connected devices
 * @param handle: handle to the I2C bus instance
 */
void i2c_bus_scan(i2c_bus_handle_t handle);

#endif /* __i2c_bus_h__ */
