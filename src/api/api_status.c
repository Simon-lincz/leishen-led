#include "api.h"
#include "../led_protocol.h"
#include "../logger.h"

#include <stdlib.h>
#include <string.h>

void handle_status(int fd, const char *method, const char *body, void *ctx) {
    (void)method; (void)body; (void)ctx;

    struct led_state state;
    if (led_read_state(&state) != 0) {
        LLOG_ERROR("failed to read LED state");
        api_send_error(fd, 500, "read failed");
        return;
    }

    char *json = led_state_to_json(&state);
    if (!json) {
        api_send_error(fd, 500, "json encoding failed");
        return;
    }

    size_t len = strlen(json);
    api_send_json(fd, 200, "OK", json, len);
    free(json);
}
