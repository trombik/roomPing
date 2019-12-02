idf_component_register(SRCS "main.c" "wifi_connect.c" "sntp_connect.c"
                            "task_icmp_client.c" "task_publish.c"
                            "task_ota.c"
    EMBED_TXTFILES "cert.pem" "certs/ca_cert_ota.pem"
    INCLUDE_DIRS ".")
