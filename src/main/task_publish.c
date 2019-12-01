#include <string.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include "task_publish.h"
#include "metric.h"

#define TAG "task_publish"

extern QueueHandle_t queue_statsd;

static int unix2iso(uint32_t unix_time, char *string, int size)
{
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64];
    nowtime = unix_time;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof(tmbuf), "%FT%TZ", nowtm);
    strlcpy(string, tmbuf, size);
    return 0;
}

void task_publish(void *pvParamters)
{
    ESP_LOGI(TAG, "Target IP address: %s", CONFIG_PROJECT_DEST_ADDR);
    ESP_LOGI(TAG, "Target port: %d", CONFIG_PROJECT_DEST_PORT);

    ESP_LOGI(TAG, "Starting the loop");
    while (1) {
        struct metric m;
        if (xQueueReceive(queue_statsd, &m, (TickType_t) 10 )) {
            char buf[64];
            unix2iso(m.timestamp, buf, sizeof(buf));
            ESP_LOGI(TAG, "Recieved a metric: packet_recieved %d @ %s", m.packet_recieved, buf);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
