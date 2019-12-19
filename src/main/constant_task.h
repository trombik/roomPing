#if !defined(__CONSTANT_TASK__H__)
#define __CONSTANT_TASK__H__

/* current measured freeheap
 * ESP32 184244
 * ESP8266 44536 (without icmp_client)
 *
 * configMINIMAL_STACK_SIZE
 * ESP32 2048
 * ESP8266 768
 * */

#define ICMP_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)
#define ICMP_TASK_PRIORITY (10)

#define PUBLISH_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 5)
#define PUBLISH_TASK_PRIORITY (5)

#define MQTT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)

#define ESP_PING_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)

#endif
