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

#include "task_publish.h"
#include "metric.h"
#include "util.h"

#define TAG "task_publish"
#define TAG_HANDLER "mqtt_event_handler_cb"
#define MAX_MQTT_TOPIC_LENGTH (256)
#define MAX_INFLUX_LENGTH (1024)

extern QueueHandle_t queue_metric;
static const char *TOPIC = CONFIG_PROJECT_MQTT_TOPIC;
const int MQTT_CONNECTED_BIT = BIT0;
extern int WIFI_CONNECTED_BIT;
extern const uint8_t cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t cert_pem_end[]   asm("_binary_cert_pem_end");
extern EventGroupHandle_t s_wifi_event_group;
EventGroupHandle_t mqtt_event_group;
esp_mqtt_client_handle_t client;
char device_id[] = "esp8266_001122334455";

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    static char *data_text;
    static char *topic;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG_HANDLER, "MQTT_EVENT_ERROR");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_HANDLER, "MQTT_EVENT_DATA");

        /* the first event of data */
        if (event->current_data_offset == 0) {
            ESP_LOGD(TAG_HANDLER, "topic_len: %d total_data_len: %d",
                     event->topic_len, event->total_data_len);

            /* the first event that contains topic.
             *
             * event->topic and event->data are not null-terminated C
             * string. allocate memory for topic_len + extra 1 byte for
             * '\0'.
             */
            topic = malloc(event->topic_len + 1);
            if (topic == NULL) {
                ESP_LOGE(TAG_HANDLER, "failed to malloc() on topic");
                break;
            }
            memset(topic, 0, 1);

            /* ignore return value of strlcpy(). it is almost always more
             * than event->topic_len + 1, as event->topic is not C string.
             */
            strlcpy(topic, event->topic, event->topic_len + 1);
            ESP_LOGD(TAG_HANDLER, "topic: `%s`", topic);

            data_text = malloc(event->total_data_len + 1);
            if (data_text == NULL) {
                ESP_LOGE(TAG_HANDLER, "failed to malloc(): topic `%s`",
                         topic);
                free(topic);
                topic = NULL;
                break;
            }
            memset(data_text, 0, 1);

        }

        /* the first and the rest of events */
        if (topic == NULL || data_text == NULL) {

            /* when something went wrong in parsing the first event,
             * ignore the rest of the events
             */
            break;
        }
        strlcat(data_text, event->data, event->data_len + 1);

        /* the last event */
        if (event->current_data_offset + event->data_len >= event->total_data_len) {

            if (topic == NULL || data_text == NULL) {
                goto free;
            }
            ESP_LOGI(TAG_HANDLER, "topic: `%s` data: `%s`", topic, data_text);
free:
            free(topic);
            topic = NULL;
            free(data_text);
            data_text = NULL;
        }
        break;
#if defined(CONFIG_IDF_TARGET_ESP32)
    case MQTT_EVENT_BEFORE_CONNECT:
        break;
#endif
    default:
        ESP_LOGI(TAG_HANDLER, "unknown event: event id = %d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}


static int mqtt_publish(const char *path, const char *value)
{
    const int qos = CONFIG_PROJECT_MQTT_QOS;
    const int retain = 1;
    int length = 0;
    char topic[MAX_MQTT_TOPIC_LENGTH];

    if (snprintf(topic, sizeof(topic), "%s/%s/%s", TOPIC, device_id, path) >= sizeof(topic)) {
        ESP_LOGE(TAG, "the size of topic is too small (max %d), truncated", sizeof(topic));
    }
    return esp_mqtt_client_publish(
               client,
               topic,
               value,
               length,
               qos,
               retain);
}

static void task_publish(void *pvParamters)
{
    influx_metric_t influx_metric;

    ESP_LOGI(TAG, "Starting the loop");
    while (1) {
        if (!xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, 1000)) {
            goto sleep;
        }
        if (xQueueReceive(queue_metric, &influx_metric, (TickType_t) 10 )) {
            printf("%s\n", influx_metric);
            if (mqtt_publish("influx", influx_metric) == 0) {
                ESP_LOGE(TAG, "failed to publish");
            }
        }
sleep:
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

esp_err_t task_publish_start(void)
{
    esp_mqtt_transport_t proto;
    char topic_lwt[MAX_MQTT_TOPIC_LENGTH] = "";
    const char lwt_msg[] = "lost";
    esp_err_t err;
    char mac_address[] = "00:00:00:00:00:00";

    ESP_ERROR_CHECK(get_mac_addr(mac_address, sizeof(mac_address)));
    ESP_ERROR_CHECK(get_device_id(device_id, sizeof(device_id)));

    /* XXX some homie controller, notably openhab2, does not allow arbitrary
     * root topic. use "homie" as root.
     */
    if (snprintf(topic_lwt, sizeof(topic_lwt), "%s/%s/%s", TOPIC, device_id, "$state") >= sizeof(topic_lwt)) {
        ESP_LOGE(TAG, "the size of topic_lwt is too small, truncated");
    }
    printf("MAC address: %s\n", mac_address);
    printf("MQTT device ID: %s\n", device_id);
    printf("MQTT URI: %s\n", CONFIG_PROJECT_MQTT_BROKER_URI);
    printf("MQTT LWT topic: %s\n", topic_lwt);
    printf("MQTT LWT message: %s\n", lwt_msg);

    if (strncasecmp(CONFIG_PROJECT_MQTT_BROKER_URI, "mqtts:", 6) == 0) {
        proto = MQTT_TRANSPORT_OVER_SSL;
    } else if (strncasecmp(CONFIG_PROJECT_MQTT_BROKER_URI, "mqtt:", 5) == 0) {
        proto = MQTT_TRANSPORT_OVER_TCP;
    } else if (strncasecmp(CONFIG_PROJECT_MQTT_BROKER_URI, "ws:", 3) == 0) {
        proto = MQTT_TRANSPORT_OVER_WS;
    } else if (strncasecmp(CONFIG_PROJECT_MQTT_BROKER_URI, "wss:", 4) == 0) {
        proto = MQTT_TRANSPORT_OVER_WSS;
    } else {
        ESP_LOGE(TAG, "Unknown URI: `%s`", CONFIG_PROJECT_MQTT_BROKER_URI);
        goto fail;
    }

    esp_mqtt_client_config_t config = {
        .uri = CONFIG_PROJECT_MQTT_BROKER_URI,
        .lwt_topic = topic_lwt,
        .lwt_msg = lwt_msg,
        .lwt_retain = 1,
        .lwt_qos = 1,
        .lwt_msg_len = 4
    };

    switch (proto) {
    case MQTT_TRANSPORT_OVER_TCP:
    case MQTT_TRANSPORT_OVER_WS:
        break;
    case MQTT_TRANSPORT_OVER_WSS:
    case MQTT_TRANSPORT_OVER_SSL:
        config.cert_pem = (const char *)cert_pem_start;
        break;
    default:
        ESP_LOGE(TAG, "Unknown MQTT_TRANSPORT_OVER_: %d", proto);
        goto fail;
    }

    mqtt_event_group = xEventGroupCreate();
    if (mqtt_event_group == NULL) {
        ESP_LOGE(TAG, "xEventGroupCreate() failed");
        goto fail;
    }

    if ((client = esp_mqtt_client_init(&config)) == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init() failed");
        goto fail;
    }
    if ((err = esp_mqtt_client_register_event(
                   client,
                   ESP_EVENT_ANY_ID,
                   mqtt_event_handler,
                   client)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_register_event(): %s",
                 esp_err_to_name(err));
        goto fail;
    }

    while (1) {
        if (xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 1000)) {
            break;
        }
    }
    if ((err = esp_mqtt_client_start(client)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_start() failed: %s", esp_err_to_name(err));
        goto fail;
    }
    if (xTaskCreate(task_publish,
                    "task_publish",
                    configMINIMAL_STACK_SIZE * 10,
                    NULL,
                    5,
                    NULL) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate() failed");
        goto fail;
    }
    ESP_LOGI(TAG, "started task_mqtt_client");
    return ESP_OK;
fail:
    return ESP_FAIL;
}
