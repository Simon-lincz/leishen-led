#ifndef LOGGER_H
#define LOGGER_H

#include <syslog.h>

#define LLOG_DEBUG(fmt, ...) syslog(LOG_DEBUG, "[%s] " fmt, __func__, ##__VA_ARGS__)
#define LLOG_INFO(fmt, ...)  syslog(LOG_INFO, "[%s] " fmt, __func__, ##__VA_ARGS__)
#define LLOG_WARN(fmt, ...)  syslog(LOG_WARNING, "[%s] " fmt, __func__, ##__VA_ARGS__)
#define LLOG_ERROR(fmt, ...) syslog(LOG_ERR, "[%s] " fmt, __func__, ##__VA_ARGS__)

void logger_init(const char *ident);
void logger_cleanup(void);

#endif
