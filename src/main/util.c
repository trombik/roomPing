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

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

esp_err_t get_device_id(char *buf, size_t len)
{
    uint8_t mac[6];

    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));

    /* the latest homie spec does not allow `_` in topic path */
    if (snprintf(buf, len, "esp-%02x%02x%02x%02x%02x%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) >= len) {
        ESP_LOGE("get_device_id", "len is too small");
        goto fail;
    }
    return ESP_OK;
fail:
    return ESP_FAIL;
}

esp_err_t get_mac_addr(char *buf, size_t len)
{
    uint8_t mac[6];
    char mac_address[] = "00:00:00:00:00:00";
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    if (snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) >= sizeof(mac_address)) {
        ESP_LOGE("get_mac_addr", "len is too small");
        goto fail;
    }
    return ESP_OK;
fail:
    return ESP_FAIL;
}
