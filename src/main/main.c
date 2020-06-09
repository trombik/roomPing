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

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_partition.h>
#include <esp_flash_partitions.h>
#include <esp_ota_ops.h>

#include "esp_idf_lib_helpers.h"
#include "wifi_connect.h"
#include "sntp_connect.h"
#include "task_icmp_client.h"
#include "task_publish.h"
#include "target.h"
#include "metric.h"
#include "constant_task.h"

#if HELPER_TARGET_IS_ESP32 && HELPER_TARGET_VERSION < HELPER_TARGET_VERSION_ESP32_V4
#error esp-idf must be version 4.0 or newer
#endif

#define HASH_SHA256_LEN (32)
#define NOT_WAIT_FOR_ALL_BITS pdFALSE
#define NOT_CLEAR_ON_EXIT pdFALSE
#define WIFI_CONNECTED_WAIT_TICK (1000 / portTICK_PERIOD_MS)

int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "app_main";
EventGroupHandle_t s_wifi_event_group;
QueueHandle_t queue_metric;

#if defined(CONFIG_IDF_TARGET_ESP32)
static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_SHA256_LEN * 2 + 1];
    hash_print[HASH_SHA256_LEN * 2] = 0;
    for (int i = 0; i < HASH_SHA256_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static void show_digests()
{
    uint8_t sha_256[HASH_SHA256_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
    return;
}

static bool diagnostic(void)
{
    bool diagnostic_is_ok = true;

    /* do diagnostic here */
    return diagnostic_is_ok;
}

static void test_firmware()
{
    bool diagnostic_is_ok;
    const esp_partition_t *running;
    esp_err_t err;

    running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if ((err = esp_ota_get_state_partition(running, &ota_state)) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {

            /* run diagnostic function ... */
            diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();

                /* NOT REACHED */
            }
        }
    } else {
        ESP_LOGW(TAG, "esp_ota_get_state_partition() failed: %s", esp_err_to_name(err));
    }
    return;
}
#endif // defined(CONFIG_IDF_TARGET_ESP32)

void app_main()
{
    time_t now;
    struct tm timeinfo;
    char time_string[64];
    BaseType_t r;
    esp_err_t err;

    /* uncomment when you enable debug log
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_DEBUG);
    esp_log_level_set("OUTBOX", ESP_LOG_DEBUG);
    esp_log_level_set("HOMIE", ESP_LOG_DEBUG);
    esp_log_level_set("HOMIE", ESP_LOG_DEBUG);
    */

#if defined(CONFIG_IDF_TARGET_ESP32)
    show_digests();
    test_firmware();
#endif // defined(CONFIG_IDF_TARGET_ESP32)
    ESP_LOGI(TAG, "Initializing NVS");
    err = nvs_flash_init();
#if defined(CONFIG_IDF_TARGET_ESP32)
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
#elif defined(CONFIG_IDF_TARGET_ESP8266)
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
#endif // defined(CONFIG_IDF_TARGET_ESP32)

        /* OTA app partition table has a smaller NVS partition size than the
         * non-OTA partition table. This size mismatch may cause NVS
         * initialization to fail.  If this happens, we erase NVS partition
         * and initialize NVS again.
         */
        ESP_LOGI(TAG, "Re-Initializing NVS");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Configuring WiFi");
    ESP_ERROR_CHECK(init_wifi());
    ESP_LOGI(TAG, "Configured WiFi. Waiting for WIFI_CONNECTED_BIT...");
    while (1) {
        if (xEventGroupWaitBits(s_wifi_event_group,
                                WIFI_CONNECTED_BIT,
                                NOT_WAIT_FOR_ALL_BITS,
                                NOT_CLEAR_ON_EXIT,
                                WIFI_CONNECTED_WAIT_TICK)) {
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
    ESP_LOGI(TAG, "Configured time");

    ESP_LOGI(TAG, "Creating queue for metrics");
    queue_metric = xQueueCreate(CONFIG_PROJECT_METRIC_QUEUE_SIZE, sizeof(influx_metric_t));
    if (queue_metric == NULL) {
        ESP_LOGE(TAG, "xQueueCreate() failed");
        goto fail;
    }


    ESP_LOGI(TAG, "Creating task_publish");
    ESP_ERROR_CHECK(task_publish_start());

    ESP_LOGI(TAG, "Creating task_icmp_client");
    for (int i = 0; i < N_TARGETS; i++) {
        r = xTaskCreate(task_icmp_client, "task_icmp_client",
                        ICMP_TASK_STACK_SIZE,
                        (void *)targets[i],
                        ICMP_TASK_PRIORITY,
                        NULL);
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
