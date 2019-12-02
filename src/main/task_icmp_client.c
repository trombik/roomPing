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
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <ping/ping_sock.h>
#include <lwip/netdb.h>
#include <string.h>

#include "metric.h"

#define TAG "task_icmp_client"

struct icmp_metric {
    ip_addr_t target_addr;
    char target[MAX_TARGET_STRING_SIZE];
    uint32_t packet_lost;
    uint32_t packet_sent;
    uint32_t packet_recieved;
    uint32_t round_trip_average;
    time_t tv_sec;
};

extern QueueHandle_t queue_metric;

static void icmp_callback_on_ping_end(esp_ping_handle_t handle, void *args)
{
    const char task_name[] = "on_ping_end";
    const char metric_name[] = "icmp";
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    uint32_t loss;
    ip_addr_t target_addr;
    char target_addr_str[64];
    influx_metric_t influx_line;
    struct icmp_metric *icmp_m;

    icmp_m = (struct icmp_metric *)args;
    esp_ping_get_profile(handle, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(handle, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(handle, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);

    if (IP_IS_V4(&target_addr)) {
        strncpy(target_addr_str, inet_ntoa(*ip_2_ip4(&target_addr)), sizeof(target_addr_str));
    } else if (IP_IS_V6(&target_addr)) {
        strncpy(target_addr_str, inet6_ntoa(*ip_2_ip6(&target_addr)), sizeof(target_addr_str));
    } else {
        ESP_LOGE(task_name, "target_addr is not IPv4 nor IPv6");
        goto fail;
    }
    printf("%s: ", target_addr_str);
    icmp_m->packet_sent = transmitted;
    icmp_m->packet_recieved = received;
    icmp_m->packet_lost = transmitted - received;

    /* packet loss
     * influxdb requires nanosecond, or int64_t, for timestamp field.
     * but second-precision is good enough for this use case
     */
    snprintf(influx_line, sizeof(influx_line),
             "%s, target_host=%s,target_addr=%s packet_loss=%d %ld000000000",
             metric_name,
             icmp_m->target,
             target_addr_str,
             loss,
             icmp_m->tv_sec);

    if (xQueueSend(queue_metric, &influx_line, (TickType_t) 0) != pdTRUE) {
        ESP_LOGW(task_name, "xQueueSend() failed");
        goto fail;
    }

    /* average */
    snprintf(influx_line, sizeof(influx_line),
             "%s, target_host=%s,target_addr=%s average=%d %ld000000000",
             metric_name,
             icmp_m->target,
             target_addr_str,
             icmp_m->round_trip_average,
             icmp_m->tv_sec);
    if (xQueueSend(queue_metric, &influx_line, (TickType_t) 0) != pdTRUE) {
        ESP_LOGW(task_name, "xQueueSend() failed");
        goto fail;
    }
    printf("%d packets transmitted, %d received, %d%% packet loss, average %dms\n",
           transmitted, received, loss, icmp_m->round_trip_average);
fail:
    return;
}

static void icmp_callback_on_ping_success(esp_ping_handle_t handle, void *args)
{
    uint32_t elapsed_time;
    uint32_t received;
    struct icmp_metric *m;

    m = (struct icmp_metric *)args;
    esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    esp_ping_get_profile(handle, ESP_PING_PROF_REPLY, &received, sizeof(received));
    m->round_trip_average = (m->round_trip_average * received + elapsed_time) / (received + 1);
    return;
}

static void icmp_callback_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    return;
}

void task_icmp_client(void *pvParamters)
{
    char *hostname;
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    struct icmp_metric m;
    int err;
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    esp_ping_callbacks_t callback = {
        .on_ping_end = icmp_callback_on_ping_end,
        .on_ping_success = icmp_callback_on_ping_success,
        .on_ping_timeout = icmp_callback_on_ping_timeout,
        .cb_args = &m
    };
    esp_ping_handle_t ping;
    const int interval_ms = 1000 * 60; // 1 minute
    TickType_t interval_tick = pdMS_TO_TICKS(interval_ms);
    TickType_t last_wakeup_time_in_tick;

    hostname = (char *)pvParamters;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));

    ESP_LOGI(hostname, "Configuring ping client with target `%s`", hostname);
    if ((err = getaddrinfo(hostname, NULL, &hint, &res)) != 0) {
        ESP_LOGE(hostname, "cannot resolve host `%s` to address: error code: 0x%x ", hostname, err);
        goto fail;
    }
    if (res->ai_family == AF_INET) {
        struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    } else if (res->ai_family == AF_INET6) {
        struct in6_addr addr6 = ((struct sockaddr_in6 *) (res->ai_addr))->sin6_addr;
        inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
    } else {
        ESP_LOGE(hostname, "getaddrinfo(): unknown ai_family");
        goto fail;
    }
    freeaddrinfo(res);
    config.target_addr = target_addr;
    m.target_addr = target_addr;
    strlcpy(m.target, hostname, sizeof(m.target));

    ESP_LOGI(hostname, "Creating new ICMP session");
    ESP_ERROR_CHECK(esp_ping_new_session(&config, &callback, &ping));
    ESP_LOGI(hostname, "Starting the loop");
    while (1) {
        struct timeval tp;

        esp_ping_stop(ping);

        gettimeofday(&tp, NULL);
        m.packet_lost = 0;
        m.packet_sent = 0;
        m.packet_recieved = 0;
        m.round_trip_average = 0;
        m.tv_sec = tp.tv_sec;

        ESP_LOGI(hostname, "Starting ICMP seesion");
        if (esp_ping_start(ping) != ESP_OK) {
            ESP_LOGE(TAG, "failed to esp_ping_start()");
            goto sleep;
        }
sleep:
        vTaskDelayUntil(&last_wakeup_time_in_tick, interval_tick);
        last_wakeup_time_in_tick = xTaskGetTickCount();
    }

fail:
    ESP_LOGE(TAG, "Deleting the task");
    freeaddrinfo(res);
    vTaskDelete(NULL);
}
