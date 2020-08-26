PROJECT_NAME := roomPing
EXTRA_COMPONENT_DIRS += components/esp-idf-lib/components

ifeq ($(PROJECT_TARGET), esp8266)
# the drivers in esp-idf-lib that do not compile on ESP8266
EXCLUDE_COMPONENTS += max7219 mcp23x17

# exclude `mqtt` component in ESP8266 RTOS SDK, which has been lagged behind,
# and broken. use `esp-mqtt` under components directory
EXCLUDE_COMPONENTS += mqtt
else
EXCLUDE_COMPONENTS += esp-mqtt
endif

include $(IDF_PATH)/make/project.mk
