PROJECT_NAME := roomPing

# exclude `mqtt` component in ESP8266 RTOS SDK, which has been lagged behind,
# and broken. use `esp-mqtt` under components directory
ifeq ($(PROJECT_TARGET), esp8266)
EXCLUDE_COMPONENTS = mqtt
else
EXCLUDE_COMPONENTS = esp-mqtt
endif

include $(IDF_PATH)/make/project.mk
