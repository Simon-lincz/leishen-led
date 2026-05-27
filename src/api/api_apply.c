#include "api.h"
#include "../led_protocol.h"
#include "../logger.h"

#include <stdlib.h>
#include <string.h>

void handle_apply(int fd, const char *method, const char *body, void *ctx) {
    (void)method;

    struct api_dispatch_ctx *actx = (struct api_dispatch_ctx *)ctx;
    if (!body) {
        api_send_error(fd, 400, "missing body");
        return;
    }

    struct led_state state;
    if (led_state_from_json(body, &state) != 0) {
        LLOG_WARN("invalid apply request: %s", body);
        api_send_error(fd, 400, "invalid json");
        return;
    }

    if (led_apply_state(&state) != 0) {
        LLOG_ERROR("failed to apply LED state");
        api_send_error(fd, 500, "apply failed");
        return;
    }

    // 异步保存状态
    if (actx && actx->schedule_save && actx->state_path) {
        actx->schedule_save(actx->state_path, &state);
    }

    char *json = led_state_to_json(&state);
    if (json) {
        api_send_json(fd, 200, "OK", json, strlen(json));
        free(json);
        return;
    }

    api_send_error(fd, 500, "json encoding failed");
}
