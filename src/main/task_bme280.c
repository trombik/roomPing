/*
 * Copyright (c) 2020 Tomoyuki Sakurai <y@trombik.org>
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
#include <esp_log.h>
#include <esp_err.h>
#include <bmp280.h>

#include "metric.h"

#if defined(CONFIG_IDF_TARGET_ESP8266)
#define SDA_GPIO 4 // D2
#define SCL_GPIO 5 // D1
#else
#define SDA_GPIO 16
#define SCL_GPIO 17
#endif

#define INTERVAL_MS (60 * 1000 * 5)
#define TASK_BME_POLLER_STACK_SIZE (configMINIMAL_STACK_SIZE * 8)
#define TAG "bme_poller"

extern QueueHandle_t queue_metric_bme280;

static bmp280_t *
find_bme280()
{
    bmp280_params_t params;
    bmp280_t *dev;

    bmp280_init_default_params(&params);
    params.mode = BMP280_MODE_FORCED;
    if ((dev = malloc(sizeof(bmp280_t))) == NULL) {
        ESP_LOGE(TAG, "mailloc()");
        goto fail;
    }
    memset(dev, 0, sizeof(bmp280_t));

    ESP_LOGI(TAG, "Initializing the desciptor");
    ESP_LOGI(TAG, "I2C address: 0x%2x, SDA GPIO: %d, SCL GPIO: %d", BMP280_I2C_ADDRESS_0, SDA_GPIO, SCL_GPIO);
    if (bmp280_init_desc(dev, BMP280_I2C_ADDRESS_0, 0, SDA_GPIO, SCL_GPIO) != ESP_OK) {
        free(dev);
        dev = NULL;
    }

    if (bmp280_init(dev, &params) != ESP_OK) {
        free(dev);
        dev = NULL;
    }
fail:
    return dev;
}

void
bme_poller(void *pvParamters)
{
    bool is_bme280;
    float pressure, temperature, humidity;
    humidity = 0;
    esp_err_t err;
    bme280_metric_t metric;
    bmp280_t *dev;

    ESP_LOGI(TAG, "Initializing the chip");
    if ((dev = find_bme280()) == NULL) {
        ESP_LOGW(TAG, "BME280 not found");
        goto notfound;
    }
    is_bme280 = dev->id == BME280_CHIP_ID;
    ESP_LOGI(TAG, "Chip ID: %s\n", is_bme280 ? "BME280" : "BMP280");

    vTaskDelay(30 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Starting the loop");
    while (1) {
        bmp280_force_measurement(dev);
        vTaskDelay(30 / portTICK_PERIOD_MS);

        if ((err = bmp280_read_float(dev, &temperature, &pressure, &humidity)) != ESP_OK) {
            ESP_LOGE(TAG, "bmp280_read_float(): 0x%x", err);
            continue;
        }
        metric.t = temperature;

        /* BMP280 does not have humidity */
        metric.h = is_bme280 ? humidity : -1;
        metric.p = pressure;
        if (xQueueSend(queue_metric_bme280, &metric, (10 / portTICK_PERIOD_MS)) != pdPASS) {
            ESP_LOGE(TAG, "xQueueSend()");
        }
        vTaskDelay(INTERVAL_MS / portTICK_PERIOD_MS);
    }
notfound:
    ESP_LOGW(TAG, "exiting");
    vTaskDelete(NULL);
}

BaseType_t
init_bme280(void)
{
    return xTaskCreatePinnedToCore(bme_poller, "bme_poller", TASK_BME_POLLER_STACK_SIZE, NULL, 5, NULL, APP_CPU_NUM);
}
