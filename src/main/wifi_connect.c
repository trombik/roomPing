/*
   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_log.h>
#include <mqtt_client.h>

#define TAG "wifi_connect"
#define MAXIMUM_RETRY 10

static int s_retry_num = 0;
extern int WIFI_CONNECTED_BIT;
extern EventGroupHandle_t s_wifi_event_group;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP");
        }
        ESP_LOGI(TAG, "Failed to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_retry_num = 0;
        ESP_LOGI(TAG, "Connected to the AP");
    }
}

esp_err_t init_wifi()
{
    esp_err_t err = ESP_FAIL;
    s_wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_PROJECT_WIFI_SSID,
            .password = CONFIG_PROJECT_WIFI_PASSWORD
        },
    };

    ESP_LOGI(TAG, "Connecting to SSID: `%s`", CONFIG_PROJECT_WIFI_SSID);

    esp_netif_init();
    if ((err = esp_event_loop_create_default()) != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_loop_create_default()");
        goto fail;
    }
    esp_netif_create_default_wifi_sta();
    if ((err = esp_wifi_init(&cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init()");
        goto fail;
    }

    if ((err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register()");
        goto fail;
    }
    if ((err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register()");
        goto fail;
    }
    if ((err = esp_wifi_set_mode(WIFI_MODE_STA)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode()");
        goto fail;
    }
    if ((err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config()");
        goto fail;
    }
    if ((err = esp_wifi_start()) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start()");
        goto fail;
    }

    err = ESP_OK;
fail:
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
    }
    return err;
}
