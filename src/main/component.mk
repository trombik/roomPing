COMPONENT_ADD_INCLUDEDIRS = .
COMPONENT_OBJS :=  main.o \
	sntp_connect.o \
	task_icmp_client.o \
	task_publish.o

ifdef CONFIG_IDF_TARGET_ESP8266
COMPONENT_OBJS += wifi_connect_esp8266.o
endif

ifdef CONFIG_IDF_TARGET_ESP32
COMPONENT_OBJS += wifi_connect_esp32.o
endif

ifdef CONFIG_PROJECT_TLS_OTA
ifndef CONFIG_PROJECT_TLS_OTA_NO_VERIFY
COMPONENT_EMBED_TXTFILES += certs/ota/ota.pem
endif
endif

ifdef CONFIG_PROJECT_TLS_MQTT
ifndef CONFIG_PROJECT_TLS_MQTT_NO_VERIFY
COMPONENT_EMBED_TXTFILES += certs/mqtt/mqtt.pem
endif
endif
