#if !defined(__METRIC__H__)
#define __METRIC__H__

#include <lwip/netdb.h>

#define MAX_TARGET_STRING_SIZE (256)

struct metric {
    ip_addr_t target_addr;
    char target[MAX_TARGET_STRING_SIZE];
    uint32_t packet_lost;
    uint32_t packet_sent;
    uint32_t packet_recieved;
    uint32_t round_trip_average;
    time_t tv_sec;
};
#endif
