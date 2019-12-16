/*
 * Copyright (c) 2019 Tomoyuki Sakurai <y@trombik.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sntp.h>
#include <esp_err.h>
#include <esp_log.h>

#define TAG "init_sntp"
#define SNTP_TASK_WAIT_RETRY (10)
#define SNTP_TASK_WAIT_DELAY (2000 / portTICK_PERIOD_MS)
#define SNTP_TASK_HOSTNAME  CONFIG_PROJECT_SNTP_HOST

static void callback_time_sync_notification(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time has been synced");
}

esp_err_t init_sntp(void)
{
    const int retry_count = SNTP_TASK_WAIT_RETRY;
    const char server_name[] = SNTP_TASK_HOSTNAME;
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
        vTaskDelay(SNTP_TASK_WAIT_DELAY);
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
