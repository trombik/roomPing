if(CONFIG_PROJECT_TLS_OTA)
    if(NOT CONFIG_PROJECT_TLS_OTA_NO_VERIFY)
        list(APPEND CERTS "certs/ota/ota.pem")
    endif()
endif()

if(CONFIG_PROJECT_TLS_MQTT)
    if(NOT CONFIG_PROJECT_TLS_MQTT_NO_VERIFY)
        list(APPEND CERTS "certs/mqtt/mqtt.pem")
    endif()
endif()

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND WIFI_CONNECT_SRCS "wifi_connect_esp32.c")
endif()

if(CONFIG_IDF_TARGET_ESP8266)
    list(APPEND WIFI_CONNECT_SRCS "wifi_connect_esp8266.c")
endif()

idf_component_register(SRCS "main.c"
                            "sntp_connect.c"
                            "task_icmp_client.c"
                            "task_publish.c"
                            ${WIFI_CONNECT_SRCS}
    EMBED_TXTFILES ${CERTS}
    REQUIRES "esp-homie" "nvs_flash" "mqtt" "esp_https_ota" "esp_wifi"
    INCLUDE_DIRS ".")
