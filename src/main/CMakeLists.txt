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

idf_component_register(SRCS "main.c"
                            "wifi_connect.c"
                            "sntp_connect.c"
                            "task_icmp_client.c"
                            "task_publish.c"
    EMBED_TXTFILES ${CERTS}
    REQUIRES "esp32-homie" "nvs_flash" "mqtt" "esp_https_ota" "esp_wifi"
    INCLUDE_DIRS ".")
