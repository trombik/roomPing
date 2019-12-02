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
#include <freertos/queue.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

#include "metric.h"
#include "util.h"

#define SCAN_BLOCKING true
#define SCAN_NON_BLOCKING false
#define DEFAULT_SCAN_LIST_SIZE (10)
#define TAG "task_wifi_scan"

extern QueueHandle_t queue_metric;

static void task_wifi_scan(void *pvParameters)
{
    int interval_ms = 1000 * 60; // one minute
    char metric_name[] = "wifi_rssi";
    char device_id[] = "esp8266_001122334455";
    uint16_t ap_count = 0;
    uint16_t n_access_point = DEFAULT_SCAN_LIST_SIZE;
    struct timeval now;
    wifi_scan_config_t config;
    TickType_t last_wakeup_time_in_tick;
    TickType_t interval_tick;
    wifi_ap_record_t ap_record[n_access_point];
    influx_metric_t m;
    esp_err_t err;

    interval_tick = pdMS_TO_TICKS(interval_ms);
    config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    config.scan_time.passive = 1000;
    ESP_ERROR_CHECK(get_device_id(device_id, sizeof(device_id)));

    ESP_LOGI(TAG, "Starting the loop");
    while (1) {
        ESP_LOGI(TAG, "Starting scan");
        esp_wifi_scan_start(&config, true);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "Done scanning");
        gettimeofday(&now, NULL);
        err = esp_wifi_scan_get_ap_num(&n_access_point);
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "esp_wifi_scan_get_ap_num(): %s", esp_err_to_name(err));
            goto sleep;
        }
        if ((err = esp_wifi_scan_get_ap_records(&n_access_point, ap_record)) != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records(): %s", esp_err_to_name(err));
            goto sleep;
        }
        if ((err = esp_wifi_scan_get_ap_num(&ap_count)) != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_scan_get_ap_num(): %s", esp_err_to_name(err));
        }
        for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
            int n;

            n = snprintf(m, sizeof(m),
                         "%s, ssid=%s,device_id=%s rssi=%d %ld000000000",
                         metric_name,
                         ap_record[i].ssid,
                         device_id,
                         ap_record[i].rssi,
                         now.tv_sec);
            if (n >= sizeof(m)) {
                ESP_LOGE(TAG, "snprintf(): too long");
                goto sleep;
            }

            printf("%s\n", m);
            if (xQueueSend(queue_metric, m, (TickType_t) 0) != pdTRUE) {
                ESP_LOGE(TAG, "xQueueSend() failed");
                goto sleep;
            }
        }
sleep:
        vTaskDelayUntil(&last_wakeup_time_in_tick, interval_tick);
        last_wakeup_time_in_tick = xTaskGetTickCount();
    }
}

esp_err_t start_task_wifi_scan(void *pvParamters)
{
    BaseType_t err;
    err = xTaskCreate(&task_wifi_scan, "task_wifi_scan", 8192 * 12, NULL, 5, NULL);
    return err == pdPASS ? ESP_OK : ESP_FAIL;
}
