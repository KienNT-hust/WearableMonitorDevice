#ifndef HEALTH_DATA_H
#define HEALTH_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include <stdint.h>

typedef enum
{
    HEALTH_EVENT_VITALS,
    HEALTH_EVENT_STEP_COUNT,
    HEALTH_EVENT_BLOOD_PRESSURE,
} health_event_type_t;

typedef struct
{
    health_event_type_t type;
    int64_t timestamp_ms;
    union
    {
        struct
        {
            uint8_t spo2;
            uint8_t heart_rate;
            bool wrist_detected;
        } vitals;
        uint32_t step_count;
        struct
        {
            uint16_t systolic;
            uint16_t diastolic;
        } blood_pressure;
    } data;
} health_event_t;

#define HEALTH_DATA_QUEUE_LEN 10
extern QueueHandle_t g_health_data_queue;
esp_err_t health_data_init(void);

#endif // HEALTH_DATA_H
