// components/health_data/health_data.c
#include "health_data.h"
#include "esp_log.h"

static const char *TAG = "health_data";

QueueHandle_t g_health_data_queue = NULL;

esp_err_t health_data_init(void)
{
    g_health_data_queue = xQueueCreate(HEALTH_DATA_QUEUE_LEN, sizeof(health_event_t));
    if (g_health_data_queue == NULL)
    {
        ESP_LOGE(TAG, "Tạo health_data_queue thất bại");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "health_data_queue sẵn sàng (depth=%d, item=%d bytes)",
             HEALTH_DATA_QUEUE_LEN, (int)sizeof(health_event_t));
    return ESP_OK;
}