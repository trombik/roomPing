# CHANGELOG

## Release 1.1.0

* 5eff8f0 bugfix: lint, remove stale CHANGELOG
* 7f1cc5e bugfix: lint
* 78d306c bugfix: lint
* 64a3093 bugfix: lint
* 224af48 bugfix: lint
* e65dcfa ci: import yamllint
* 36e1d96 ci: inport markdownlint-cli
* fb4bf64 bugfix: remove PROJECT_DEVICE_LOCATION
* 9f411c0 doc: update README
* a3da2f1 bugfix: update esp-homie to he latest
* 9fbe8f9 doc: update demo screenshot
* c411903 bugfix: s/esp32-/esp/
* 7231f41 doc: document PROJECT_TARGET
* 0c31c11 doc: document the crash
* fdc054c import esp-homie
* fef85a9 remove old esp32-homie
* 48116ad doc: add Hardware notes for ESP-01

## Release 1.0.1

* 3b9a29d bugfix: remove log
* dbf9ab7 bugfix: increase stack for icmp task

## Release 0.3.0

* 4046520 doc: document use cases, required softwares
* 5ad74f2 doc: document not-a-bug
* 879854e bugfix: require users to create their own `target.h`
* 0f5c491 doc: add a link to Kconfig
* 79a4050 bugfix: introduce PROJECT_OTA_ENBALED and PROJECT_REBOOT_ENBALED
* 0ac4e73 bugfix: remove magic numbers
* f314ef1 bugfix: remove unused macro
* c98fa64 bugfix: remove unused macro
* 6e207b2 doc: remove statements which are not longer true

## Release 0.2.0

* 5d354a9 doc: add build status
* 2c13a84 doc: update README
* 5c9949a bugfix: remove unused macros
* b505bc2 bugfix: add `icmp` to node_lists
* 2ae6b9a bugfix: clarify the `certs`
* 0e0c309 bugfix: use homie_publish() instead of esp_mqtt_client_publish()
* 58326cf bugfix: remove unused macro
* dc2107b bugfix: remove task_ota.o from component.mk
* 83394a0 bugfix: support Homie 4.x
* 9efef4e wip: import esp32-homie
* 21dbc93 bugfix: add missing depending components
