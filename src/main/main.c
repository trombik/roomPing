#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "esp_idf_lib_helpers.h"
#include "wifi_connect.h"
#include "sntp_connect.h"
#include "task_icmp_client.h"
#include "task_publish.h"
#include "metric.h"

#if !HELPER_TARGET_IS_ESP32
#error The target must be ESP32
#endif

#if HELPER_TARGET_VERSION < HELPER_TARGET_VERSION_ESP32_V4
#error esp-idf must be version 4.0 or newer
#endif

#define N_TARGETS 2
const char targets[N_TARGETS][MAX_TARGET_STRING_SIZE] = {
    "8.8.8.8",
    "yahoo.co.jp",
};

int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "app_main";
EventGroupHandle_t s_wifi_event_group;
QueueHandle_t queue_statsd;

void app_main()
{
    time_t now;
    struct tm timeinfo;
    char time_string[64];
    BaseType_t r;
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing NVS");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Configuring WiFi");
    ESP_ERROR_CHECK(init_wifi());
    ESP_LOGI(TAG, "Configured WiFi. Waiting for WIFI_CONNECTED_BIT...");
    while (1) {
        if (xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 1000)) {
            break;
        }
    }

    ESP_LOGI(TAG, "Configuring time");
    if (time(&now) == -1) {
        ESP_LOGI(TAG, "time()");
    }
    localtime_r(&now, &timeinfo);
    strftime(time_string, sizeof(time_string), "%FT%TZ", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", time_string);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Setting time over NTP");
        if ((err = init_sntp()) != ESP_OK) {
            ESP_LOGE(TAG, "init_sntp(): %x", err);
            goto fail;
        }
    }

    ESP_LOGI(TAG, "Creating queue for metrics");
    queue_statsd = xQueueCreate(10, sizeof(struct metric));
    if (queue_statsd == 0) {
        ESP_LOGE(TAG, "xQueueCreate() failed");
        goto fail;
    }

    ESP_LOGI(TAG, "Creating task_publish");
    r = xTaskCreate(task_publish, "task_publish", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
    if (r != pdPASS) {
        ESP_LOGE(TAG, "failed to create task_publish");
        goto fail;
    }

    ESP_LOGI(TAG, "Creating task_icmp_client");
    for (int i = 0; i < N_TARGETS; i++) {
        r = xTaskCreate(task_icmp_client, "task_icmp_client", configMINIMAL_STACK_SIZE * 3, (void *)targets[i], 5, NULL);
        if (r != pdPASS) {
            ESP_LOGE(TAG, "failed to create task_icmp_client with target %s", targets[i]);
            goto fail;
        }
    }
    return;
fail:
    ESP_LOGE(TAG, "Critical error, aborting");
    abort();
}
