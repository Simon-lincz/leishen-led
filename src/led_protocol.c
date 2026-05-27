#include "led_protocol.h"

#include "ec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mode_name {
    const char *name;
    uint8_t value;
};

static const struct mode_name modes[] = {
    {"off", 0},
    {"static", 1},
    {"breathing", 2},
    {"flow", 3},
    {"color-flow", 3},
    {"flash", 4},
    {"successively", 5},
    {"marquee", 6},
    {"scanning", 7},
    {"meteor", 8},
};

static const led_mode_info_t mode_infos[] = {
    {0, "off", "关闭", 0, 0, 0},
    {1, "static", "常亮", 1, 1, 0},
    {2, "breathing", "呼吸", 1, 1, 1},
    {3, "flow", "流光", 1, 1, 1},
    {4, "flash", "闪烁", 1, 1, 1},
    {5, "successively", "渐次", 1, 1, 1},
    {6, "marquee", "跑马灯", 1, 1, 1},
    {7, "scanning", "扫描", 1, 1, 1},
    {8, "meteor", "流星", 1, 1, 1},
};

static int parse_u8(const char *text, uint8_t *value) {
    char *end = NULL;
    long parsed = strtol(text, &end, 0);

    if (*text == '\0' || *end != '\0' || parsed < 0 || parsed > 255) {
        return -1;
    }

    *value = (uint8_t)parsed;
    return 0;
}

int led_parse_mode(const char *text, uint8_t *value) {
    if (parse_u8(text, value) == 0) {
        return *value <= 8 ? 0 : -1;
    }

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        if (strcmp(text, modes[i].name) == 0) {
            *value = modes[i].value;
            return 0;
        }
    }

    return -1;
}

const char *led_mode_label(uint8_t value) {
    switch (value) {
    case 0: return "off";
    case 1: return "static";
    case 2: return "breathing";
    case 3: return "flow";
    case 4: return "flash";
    case 5: return "successively";
    case 6: return "marquee";
    case 7: return "scanning";
    case 8: return "meteor";
    default: return "unknown";
    }
}

const led_mode_info_t *led_get_mode_info(uint8_t value) {
    if (value >= LED_MODE_MAX) {
        return NULL;
    }

    return &mode_infos[value];
}

int led_validate_state(const struct led_state *state) {
    return state && state->mode <= 8 ? 0 : -1;
}

static int json_find_int(const char *json, const char *key, int min, int max, int *out) {
    char pattern[64];
    const char *at = NULL;
    const char *cursor = NULL;
    char *end = NULL;
    long value = 0;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    at = strstr(json, pattern);
    if (!at) {
        return -1;
    }

    cursor = strchr(at + strlen(pattern), ':');
    if (!cursor) {
        return -1;
    }
    cursor++;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n') {
        cursor++;
    }

    value = strtol(cursor, &end, 10);
    if (cursor == end || value < min || value > max) {
        return -1;
    }

    *out = (int)value;
    return 0;
}

static int json_find_string(const char *json, const char *key, char *out, size_t out_size) {
    char pattern[64];
    const char *at = NULL;
    const char *cursor = NULL;
    const char *end = NULL;
    size_t length = 0;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    at = strstr(json, pattern);
    if (!at) {
        return -1;
    }

    cursor = strchr(at + strlen(pattern), ':');
    if (!cursor) {
        return -1;
    }
    cursor++;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n') {
        cursor++;
    }
    if (*cursor != '"') {
        return -1;
    }
    cursor++;

    end = strchr(cursor, '"');
    if (!end) {
        return -1;
    }

    length = (size_t)(end - cursor);
    if (length == 0 || length >= out_size) {
        return -1;
    }

    memcpy(out, cursor, length);
    out[length] = '\0';
    return 0;
}

static int json_find_color(const char *json, struct led_state *state) {
    const char *at = strstr(json, "\"color\"");
    const char *open = NULL;
    int r = 0;
    int g = 0;
    int b = 0;

    if (!at) {
        return -1;
    }
    open = strchr(at, '[');
    if (!open || sscanf(open, "[%d,%d,%d]", &r, &g, &b) != 3) {
        return -1;
    }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return -1;
    }

    state->r = (uint8_t)r;
    state->g = (uint8_t)g;
    state->b = (uint8_t)b;
    return 0;
}

char *led_state_to_json(const struct led_state *state) {
    char *json = NULL;
    int length = 0;

    if (led_validate_state(state)) {
        return NULL;
    }

    length = snprintf(NULL, 0,
        "{\"mode\":%u,\"modeName\":\"%s\",\"brightness\":%u,"
        "\"color\":[%u,%u,%u],\"time\":%u}",
        state->mode, led_mode_label(state->mode), state->brightness,
        state->r, state->g, state->b, state->time);
    if (length < 0) {
        return NULL;
    }

    json = malloc((size_t)length + 1);
    if (!json) {
        return NULL;
    }

    snprintf(json, (size_t)length + 1,
        "{\"mode\":%u,\"modeName\":\"%s\",\"brightness\":%u,"
        "\"color\":[%u,%u,%u],\"time\":%u}",
        state->mode, led_mode_label(state->mode), state->brightness,
        state->r, state->g, state->b, state->time);
    return json;
}

int led_state_from_json(const char *json, struct led_state *state) {
    char mode_text[32];
    int value = 0;

    if (!json || !state) {
        return -1;
    }

    memset(state, 0, sizeof(*state));
    if (json_find_string(json, "mode", mode_text, sizeof(mode_text)) == 0) {
        if (led_parse_mode(mode_text, &state->mode)) {
            return -1;
        }
    } else if (json_find_int(json, "mode", 0, LED_MODE_MAX - 1, &value) == 0) {
        state->mode = (uint8_t)value;
    } else {
        return -1;
    }

    if (json_find_int(json, "brightness", 0, 255, &value)) {
        return -1;
    }
    state->brightness = (uint8_t)value;

    if (json_find_color(json, state)) {
        return -1;
    }

    if (json_find_int(json, "time", 0, 65535, &value)) {
        return -1;
    }
    state->time = (uint16_t)value;

    return led_validate_state(state);
}

int led_read_state(struct led_state *state) {
    uint8_t time_h = 0;
    uint8_t time_l = 0;

    if (!state) {
        return -1;
    }

    if (ec_read(LED_MODE, &state->mode) ||
        ec_read(LED_BRIGHTNESS, &state->brightness) ||
        ec_read(LED_R, &state->r) ||
        ec_read(LED_G, &state->g) ||
        ec_read(LED_B, &state->b) ||
        ec_read(LED_TIME_H, &time_h) ||
        ec_read(LED_TIME_L, &time_l)) {
        return -1;
    }

    state->time = ((uint16_t)time_h << 8) | time_l;
    return led_validate_state(state);
}

int led_apply_state(const struct led_state *state) {
    if (led_validate_state(state)) {
        return -1;
    }

    if (ec_write(LED_MODE, state->mode) ||
        ec_write(LED_R, state->r) ||
        ec_write(LED_G, state->g) ||
        ec_write(LED_B, state->b) ||
        ec_write(LED_BRIGHTNESS, state->brightness) ||
        ec_write(LED_TIME_H, (uint8_t)(state->time >> 8)) ||
        ec_write(LED_TIME_L, (uint8_t)(state->time & 0xff))) {
        return -1;
    }

    return 0;
}
