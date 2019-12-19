#if !defined(__CONSTANT_TASK__H__)
#define __CONSTANT_TASK__H__

/* current measured freeheap 184244 */

#define ICMP_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)
#define ICMP_TASK_PRIORITY (10)

#define PUBLISH_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 5)
#define PUBLISH_TASK_PRIORITY (5)

#define MQTT_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)

#define ESP_PING_STACK_SIZE (configMINIMAL_STACK_SIZE * 3)

#endif
