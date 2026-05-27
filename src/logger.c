#include "logger.h"

void logger_init(const char *ident) {
    openlog(ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    setlogmask(LOG_UPTO(LOG_INFO));
}

void logger_cleanup(void) {
    closelog();
}
