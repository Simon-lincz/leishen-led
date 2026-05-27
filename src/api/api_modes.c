#include "api.h"
#include "../led_protocol.h"

#include <stdio.h>

void handle_modes(int fd, const char *method, const char *body, void *ctx) {
    (void)method; (void)body; (void)ctx;

    char buf[2048];
    int offset = 0;

    offset += snprintf(buf + offset, sizeof(buf) - offset, "[");

    for (int i = 0; i < LED_MODE_MAX; i++) {
        const led_mode_info_t *info = led_get_mode_info(i);
        if (!info) continue;

        if (i > 0) offset += snprintf(buf + offset, sizeof(buf) - offset, ",");

        offset += snprintf(buf + offset, sizeof(buf) - offset,
            "{\"mode\":%d,\"name\":\"%s\",\"label\":\"%s\","
            "\"has_color\":%d,\"has_brightness\":%d,\"has_time\":%d}",
            info->mode, info->name, info->label_zh,
            info->has_color, info->has_brightness, info->has_time);
    }

    offset += snprintf(buf + offset, sizeof(buf) - offset, "]");

    api_send_json(fd, 200, "OK", buf, offset);
}
