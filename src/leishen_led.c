#include "ec.h"
#include "led_protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program) {
    fprintf(stderr,
        "Usage:\n"
        "  %s status\n"
        "  %s mode <off|static|breathing|flow|flash|successively|marquee|scanning|meteor|0-8>\n"
        "  %s color <r> <g> <b>\n"
        "  %s brightness <0-255>\n"
        "  %s time <0-65535>\n"
        "  %s set [--mode M] [--color R G B] [--brightness N] [--time N]\n",
        program, program, program, program, program, program);
}

static int parse_u8(const char *text, uint8_t *value) {
    char *end = NULL;
    long parsed = strtol(text, &end, 0);

    if (*text == '\0' || *end != '\0' || parsed < 0 || parsed > 255) {
        return -1;
    }

    *value = (uint8_t)parsed;
    return 0;
}

static int parse_u16(const char *text, uint16_t *value) {
    char *end = NULL;
    long parsed = strtol(text, &end, 0);

    if (*text == '\0' || *end != '\0' || parsed < 0 || parsed > 65535) {
        return -1;
    }

    *value = (uint16_t)parsed;
    return 0;
}

static int read_current_or_default(struct led_state *state) {
    if (led_read_state(state) == 0) {
        return 0;
    }

    state->mode = 1;
    state->brightness = 70;
    state->r = 178;
    state->g = 0;
    state->b = 255;
    state->time = 0;
    return 0;
}

static int print_status(void) {
    struct led_state state;

    if (led_read_state(&state)) {
        fprintf(stderr, "read status failed\n");
        return 1;
    }

    printf("mode: %u (%s)\n", state.mode, led_mode_label(state.mode));
    printf("brightness: %u\n", state.brightness);
    printf("color: %u %u %u\n", state.r, state.g, state.b);
    printf("time: %u\n", state.time);
    return 0;
}

int main(int argc, char **argv) {
    struct led_state state;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (ec_init()) {
        return 1;
    }

    if (strcmp(argv[1], "status") == 0) {
        return print_status();
    }

    if (read_current_or_default(&state)) {
        fprintf(stderr, "read current state failed\n");
        return 1;
    }

    if (strcmp(argv[1], "mode") == 0) {
        if (argc != 3 || led_parse_mode(argv[2], &state.mode)) {
            print_usage(argv[0]);
            return 1;
        }

        return led_apply_state(&state) == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "color") == 0) {
        if (argc != 5 || parse_u8(argv[2], &state.r) ||
            parse_u8(argv[3], &state.g) || parse_u8(argv[4], &state.b)) {
            print_usage(argv[0]);
            return 1;
        }

        return led_apply_state(&state) == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "brightness") == 0) {
        if (argc != 3 || parse_u8(argv[2], &state.brightness)) {
            print_usage(argv[0]);
            return 1;
        }

        return led_apply_state(&state) == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "time") == 0) {
        if (argc != 3 || parse_u16(argv[2], &state.time)) {
            print_usage(argv[0]);
            return 1;
        }

        return led_apply_state(&state) == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "set") == 0) {
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--mode") == 0) {
                if (++i >= argc || led_parse_mode(argv[i], &state.mode)) {
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "--color") == 0) {
                if (i + 3 >= argc || parse_u8(argv[i + 1], &state.r) ||
                    parse_u8(argv[i + 2], &state.g) || parse_u8(argv[i + 3], &state.b)) {
                    print_usage(argv[0]);
                    return 1;
                }
                i += 3;
            } else if (strcmp(argv[i], "--brightness") == 0) {
                if (++i >= argc || parse_u8(argv[i], &state.brightness)) {
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "--time") == 0) {
                if (++i >= argc || parse_u16(argv[i], &state.time)) {
                    print_usage(argv[0]);
                    return 1;
                }
            } else {
                print_usage(argv[0]);
                return 1;
            }
        }

        return led_apply_state(&state) == 0 ? 0 : 1;
    }

    print_usage(argv[0]);
    return 1;
}
