#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "i2c_bus";

typedef struct
{
    i2c_port_t port;
    uint32_t timeout_ms;
    SemaphoreHandle_t mutex;
} i2c_bus_t;

i2c_bus_handle_t i2c_bus_create(const i2c_bus_config_t *config)
{
    if (config == NULL)
    {
        ESP_LOGE(TAG, "Invalid config");
        return NULL;
    }

    i2c_bus_t *bus = (i2c_bus_t *)calloc(1, sizeof(i2c_bus_t));
    if (bus == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for I2C bus");
        return NULL;
    }

    bus->port = config->port;
    bus->timeout_ms = (config->timeout_ms > 0) ? config->timeout_ms : 1000;
    bus->mutex = xSemaphoreCreateMutex();

    if (bus->mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex for I2C bus");
        free(bus);
        return NULL;
    }

    i2c_config_t i2c_conf;
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = config->sda_pin;
    i2c_conf.scl_io_num = config->scl_pin;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = config->clk_speed_hz;
    i2c_conf.clk_flags = config->clk_flags;

    esp_err_t ret = i2c_param_config(bus->port, &i2c_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure I2C bus: %s", esp_err_to_name(ret));
        goto fail;
    }

    ret = i2c_driver_install(bus->port, i2c_conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        goto fail;
    }

    ESP_LOGI(TAG, "I2C bus created successfully, PORT: %d, SDA: %d, SCL: %d, freq: %lu HZ",
             config->port, config->sda_pin, config->scl_pin, config->clk_speed_hz);
    return (i2c_bus_handle_t)bus;

fail:
    vSemaphoreDelete(bus->mutex);
    free(bus);
    return NULL;
}

void i2c_bus_destroy(i2c_bus_handle_t handle)
{
    if (handle == NULL)
    {
        ESP_LOGE(TAG, "Invalid handle");
        return;
    }

    i2c_bus_t *bus = (i2c_bus_t *)handle;
    i2c_driver_delete(bus->port);
    vSemaphoreDelete(bus->mutex);
    free(bus);
}

esp_err_t i2c_bus_read(i2c_bus_handle_t handle,
                       uint8_t dev_addr, uint8_t reg_addr,
                       uint8_t *buf, uint16_t len)
{
    if (handle == NULL || buf == NULL || len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_t *bus = (i2c_bus_t *)handle;
    if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(bus->timeout_ms)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Timeout for waiting mutex (write addr=0x%02X)", dev_addr);
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, buf + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(bus->port, cmd,
                                         pdMS_TO_TICKS(bus->timeout_ms));
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(bus->mutex);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Read fail addr=0x%02X reg=0x%02X: %s",
                 dev_addr, reg_addr, esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bus_write(i2c_bus_handle_t handle,
                        uint8_t dev_addr, uint8_t reg_addr,
                        const uint8_t *buf, uint16_t len)
{
    if (handle == NULL || buf == NULL || len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_t *bus = (i2c_bus_t *)handle;
    if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(bus->timeout_ms)) != pdTRUE)
    {
        ESP_LOGW(TAG, "Timeout for waiting mutex (write addr=0x%02X)", dev_addr);
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, buf, len, true);
    i2c_master_stop(cmd);

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(bus->mutex);
    esp_err_t err = i2c_master_cmd_begin(bus->port, cmd,
                                         pdMS_TO_TICKS(bus->timeout_ms));
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Write fail addr=0x%02X reg=0x%02X: %s",
                 dev_addr, reg_addr, esp_err_to_name(err));
    }

    return err;
}

void i2c_bus_scan(i2c_bus_handle_t handle)
{
    if (handle == NULL)
        return;
    ESP_LOGI(TAG, "Scanning I2C bus...");

    int count = 0;
    for (uint8_t addr = 0x01; addr < 0x7F; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        i2c_bus_t *bus = (i2c_bus_t *)handle;
        esp_err_t err = i2c_master_cmd_begin(bus->port, cmd,
                                             pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "  Tìm thấy thiết bị tại 0x%02X", addr);
            count++;
        }
    }

    ESP_LOGI(TAG, "Scan find: %d device", count);
}