#include "led_protocol.h"

#include "ec.h"

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

int led_validate_state(const struct led_state *state) {
    return state && state->mode <= 8 ? 0 : -1;
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
