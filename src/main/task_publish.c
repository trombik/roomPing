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
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <homie.h>

#include "task_publish.h"
#include "metric.h"

#define TAG "task_publish"
#define QOS_1 (1)
#define RETAINED (1)
#define WIFI_CONNECTED_WAIT_TICK (1000 / portTICK_PERIOD_MS)
#define NOT_WAIT_FOR_ALL_BITS pdFALSE
#define NOT_CLEAR_ON_EXIT pdFALSE
#define PUBLISH_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 20)
#define PUBLISH_TASK_PRIORITY (5)
#define PUBLISH_TASK_QUEUE_RECEIVE_TICK (100 / portTICK_PERIOD_MS)
#define MQTT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 11)
#define MQTT_TASK_KEEPALIVE_SEC (30)

#if defined(CONFIG_PROJECT_OTA_ENABLED)
#define PUBLISH_TASK_OTA_ENABLED true
#else
#define PUBLISH_TASK_OTA_ENABLED false
#endif

#if defined(CONFIG_PROJECT_REBOOT_ENABLED)
#define PUBLISH_TASK_REBOOT_ENABLED true
#else
#define PUBLISH_TASK_REBOOT_ENABLED false
#endif

extern QueueHandle_t queue_metric;
const int MQTT_CONNECTED_BIT = BIT0;
extern int WIFI_CONNECTED_BIT;

/* certificate for MQTT connection
 * uncomment when the MQTT broker in the test environment supports TLS
extern const char cert_pem_start[] asm("_binary_cert_pem_start");
extern const char cert_pem_end[]   asm("_binary_cert_pem_end");
*/

/* certificate for HTTPS OTA */
extern const char ca_cert_ota_pem_start[] asm("_binary_ca_cert_ota_pem_start");
extern const char ca_cert_ota_pem_end[] asm("_binary_ca_cert_ota_pem_end");

extern EventGroupHandle_t s_wifi_event_group;
EventGroupHandle_t mqtt_event_group;
esp_mqtt_client_handle_t client;

const esp_mqtt_client_config_t mqtt_config = {
    .client_id = NULL,
    .username = "",
    .password = "",
    .uri = CONFIG_PROJECT_MQTT_BROKER_URI,
    .task_stack = MQTT_TASK_STACK_SIZE,
    .event_loop_handle = NULL,
    .keepalive = MQTT_TASK_KEEPALIVE_SEC,
    .cert_pem = NULL,
};

const esp_http_client_config_t http_config = {
    .url = CONFIG_PROJECT_LATEST_APP_URL,
    .cert_pem =  (const char *)ca_cert_ota_pem_start,
};

homie_config_t homie_conf = {
    .mqtt_config = mqtt_config,
    .http_config = http_config,
    .device_name = "mydevice",
    .base_topic = "", // set this later
    .firmware_name = "myname",
    .firmware_version = "1",
    .ota_enabled = PUBLISH_TASK_OTA_ENABLED,
    .reboot_enabled = PUBLISH_TASK_REBOOT_ENABLED,
    .init_handler = NULL, // set this later
    .mqtt_handler = NULL,
    .ota_status_handler = NULL,
    .event_group = NULL, // set this later
    .node_lists = "icmp",
};

static void task_publish(void *pvParamters)
{
    influx_metric_t influx_metric;

    ESP_LOGI(TAG, "Starting the loop");
    while (1) {
        if (!xEventGroupWaitBits(mqtt_event_group,
                                 MQTT_CONNECTED_BIT,
                                 NOT_CLEAR_ON_EXIT,
                                 NOT_WAIT_FOR_ALL_BITS,
                                 WIFI_CONNECTED_WAIT_TICK)) {
            continue;
        }
        if (xQueueReceive(queue_metric,
                          &influx_metric,
                          PUBLISH_TASK_QUEUE_RECEIVE_TICK)) {
            printf("%s\n", influx_metric);
            if (homie_publish("icmp/influx", QOS_1, RETAINED, influx_metric) <= 0) {
                ESP_LOGE(TAG, "failed to publish");
            }
        }
    }
}

static void init_handler()
{
    if (homie_publish("icmp/$name", QOS_1, RETAINED, "ICMP statics") <= 0) {
        goto fail;
    }
    if (homie_publish("icmp/$properties", QOS_1, RETAINED, "influx") <= 0) {
        goto fail;
    }
    if (homie_publish("icmp/influx/$name", QOS_1, RETAINED, "Statistics in influx line format") <= 0) {
        goto fail;
    }
    if (homie_publish("icmp/influx/$datatype", QOS_1, RETAINED, "string") <= 0) {
        goto fail;
    }
fail:
    return;
}

esp_err_t task_publish_start(void)
{
    int ret;
    char nice_mac_address[] = "00:00:00:00:00:00";
    char mac_address[] = "aabbccddeeff";
    esp_err_t err;

    ESP_ERROR_CHECK(homie_get_mac(nice_mac_address, sizeof(nice_mac_address), true));
    ESP_ERROR_CHECK(homie_get_mac(mac_address, sizeof(mac_address), false));
    printf("MAC address: %s\n", nice_mac_address);

    ESP_LOGI(TAG, "Creating mqtt_event_group");
    mqtt_event_group = xEventGroupCreate();
    if (mqtt_event_group == NULL) {
        ESP_LOGE(TAG, "xEventGroupCreate() failed");
        goto fail;
    }
    homie_conf.event_group = &mqtt_event_group;
    homie_conf.init_handler = init_handler;

    ret = snprintf(homie_conf.base_topic, sizeof(homie_conf.base_topic), "homie/%s", mac_address);
    if (ret < 0 || ret >= sizeof(homie_conf.base_topic)) {
        ESP_LOGE(TAG, "mac_address is too long");
        goto fail;
    }

    ESP_LOGI(TAG, "Wating for WIFI_CONNECTED_BIT");
    while (1) {
        if (xEventGroupWaitBits(s_wifi_event_group,
                                WIFI_CONNECTED_BIT,
                                NOT_CLEAR_ON_EXIT,
                                NOT_WAIT_FOR_ALL_BITS,
                                WIFI_CONNECTED_WAIT_TICK)) {
            break;
        }
    }

    ESP_LOGI(TAG, "running homie_init()");
    err = homie_init(&homie_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "homie_init() failed");
        goto fail;
    }
    ESP_LOGI(TAG, "running homie_run()");
    client = homie_run();
    if (client == NULL) {
        ESP_LOGE(TAG, "homie_run()");
        goto fail;
    }
    ESP_LOGI(TAG, "Creating task_publish()");
    if (xTaskCreate(task_publish,
                    "task_publish",
                    PUBLISH_TASK_STACK_SIZE,
                    NULL,
                    PUBLISH_TASK_PRIORITY,
                    NULL) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate() failed");
        goto fail;
    }
    ESP_LOGI(TAG, "started task_mqtt_client");
    return ESP_OK;
fail:
    return ESP_FAIL;
}
