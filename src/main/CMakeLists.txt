idf_component_register(SRCS "main.c"
                            "wifi_connect.c"
                            "sntp_connect.c"
                            "task_icmp_client.c"
                            "task_publish.c"
                            "util.c"
    EMBED_TXTFILES "cert.pem" "certs/ca_cert_ota.pem"
    REQUIRES "esp32-homie" "nvs_flash" "mqtt" "esp_https_ota" "esp_wifi"
    INCLUDE_DIRS ".")
