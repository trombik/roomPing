## Release 1.0.0

* c6e136c doc: document ESP8266 support
* 4fec2ae bugfix: add missing CONFIG_LWIP_IPV6=y
* 6ed8377 bugfix: add default sdkconfig for ESP8266
* 555a671 bugfix: style
* 012cec3 ci: connect the build to CI
* 658926a imp: add Kconfig and component.mk
* d2ba76e imp: import esp-mqtt
* a0e0596 bugfix: update esp32-homie
* 5e4708f imp: log when icmp session ends for debugging
* e3c0c48 imp: add a sction to selectively enable debug log
* de6da75 bugfix: wait for MQTT connection to be established
* bc1c9bf bugfix: fix typos in th upstream
* 3980656 bugfix: update to the latest
* d537660 bugfix: record current freeheap on ESP32 and ESP8266
* 61ebbfe bugfix: adjust stack sizes
* 486daee bugfix: keep task-related constants in constant_task.h
* 7132323 bugfix: style
* d4551a8 bugfix: make the code more ESP8266-friendly
* c186009 bugfix: remove IPv6 support
* dd400ff bugfix: fix wrong variable names
* cb6971c bugfix: make the code esp8266-friendly
* 8356d7d bugfix: wrong md syntax
* e12817d doc: add a screenshot
* 7a901c9 bugfix: fix wrong order of snprintf() args
* 90d0ffa bugfix: remove duplicated esp_event_handler_register()
* 6307594 bugfix: log when re-initialization happens
* 3497673 doc: mention a note about tag values
* 5772bd5 feature: introduce `location` tag set in metrics
* d0352dd Start 0.5.0

## Release 0.4.0

* 02e34bb prepare Release 0.4.0
* 57af2ce bugfix: expose PROJECT_SNTP_HOST
* 1ef8202 bugfix: replace strncpy(3) with strlcpy(3)
* 470c719 bugfix: allow users to use their own certificates
* e874269 bugfix: s/WIFI_CONNECTED_WAIT_TICK/MQTT_CONNECTED_WAIT_TICK/
* 7a983bc bugfix: update version
* 73106f8 bugfix: set `esp/firmware-version` to running version
* 6548f4f bugfix: expose MQTT user name and password
* 260f88c bugfix: remove `static` keyword
* 18a7646 bugfix: use MAC address as part of topic
* 644c313 update esp32-homie to the latest

## Release 0.1.0

Initial release.
