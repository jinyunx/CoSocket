#ifndef SIMPLE_LOG_H
#define SIMPLE_LOG_H
#include <sys/time.h>

#pragma GCC diagnostic ignored "-Wformat"

#define SIMPLE_LOG(format, args...) \
do { \
    struct timeval tv; \
    struct tm nowtm; \
    gettimeofday(&tv, 0); \
    localtime_r(&tv.tv_sec, &nowtm); \
    printf("%d-%d-%d %d:%d:%d.%d %s:%s(%d) ", \
           nowtm.tm_year + 1900, nowtm.tm_mon + 1, \
           nowtm.tm_mday, nowtm.tm_hour, nowtm.tm_min, \
           nowtm.tm_sec, tv.tv_usec/1000, \
           __FILE__, __func__, __LINE__); \
    printf(format, ##args); \
    printf("\n"); \
} while (0)

#endif // SIMPLE_LOG_H
