#if !defined(__METRIC__H__)
#define __METRIC__H__

#include <lwip/netdb.h>

#define CONFIG_PROJECT_MAX_TARGET_STRING_SIZE (32)

#define METRIC_INFLUX_MAX_LEN (512)
typedef char influx_metric_t[METRIC_INFLUX_MAX_LEN];

#endif
