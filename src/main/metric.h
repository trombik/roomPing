#if !defined(__METRIC__H__)
#define __METRIC__H__

#include <lwip/netdb.h>

#define METRIC_INFLUX_MAX_LEN (512)
typedef char influx_metric_t[METRIC_INFLUX_MAX_LEN];

#endif
