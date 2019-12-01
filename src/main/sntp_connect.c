#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sntp.h>
#include <esp_err.h>
#include <esp_log.h>

#define TAG "init_sntp"

static void callback_time_sync_notification(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time has been synced");
}

esp_err_t init_sntp(void)
{
    const int retry_count = 10;
    const char server_name[] = "pool.ntp.org";
    esp_err_t err = ESP_FAIL;
    int retry = 0;
    time_t now;
    struct tm timeinfo;
    char time_string[64];

    ESP_LOGI(TAG, "Initializing SNTP: mode: POLL server: %s", server_name);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, server_name);
    sntp_set_time_sync_notification_cb(callback_time_sync_notification);
    sntp_init();

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if (retry >= retry_count) {
        err = ESP_ERR_TIMEOUT;
        goto fail;
    } else {
        err = ESP_OK;
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(time_string, sizeof(time_string), "%FT%TZ", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", time_string);
fail:
    return err;
}
