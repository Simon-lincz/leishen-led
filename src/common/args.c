#include "common.h"
#include "../led_protocol.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *short_options = "hvd";

static const struct option long_options[] = {
    {"help",        no_argument,       NULL, 'h'},
    {"version",     no_argument,       NULL, 'V'},
    {"verbose",     no_argument,       NULL, 'v'},
    {"daemon",      no_argument,       NULL, 'd'},
    {"mode",        required_argument, NULL, 256},
    {"color",       required_argument, NULL, 257},
    {"brightness",  required_argument, NULL, 258},
    {"time",        required_argument, NULL, 259},
    {NULL, 0, NULL, 0}
};

void args_print_usage(const char *program) {
    fprintf(stderr,
        "Usage: %s [options] <command> [command-args]\n"
        "\n"
        "Options:\n"
        "  -h, --help          Show this help\n"
        "  -V, --version       Show version\n"
        "  -v, --verbose       Verbose output\n"
        "  -d, --daemon        Run as daemon (daemon only)\n"
        "\n"
        "Commands:\n"
        "  status                      Show current LED state\n"
        "  mode <mode>                 Set mode (off/static/breathing/...)\n"
        "  color <r> <g> <b>           Set RGB color (0-255)\n"
        "  brightness <0-255>          Set brightness\n"
        "  time <0-65535>              Set time parameter\n"
        "  set [options]              Set all parameters at once\n"
        "    --mode <mode>\n"
        "    --color <r> <g> <b>\n"
        "    --brightness <0-255>\n"
        "    --time <0-65535>\n"
        "\n"
        "Modes: off(0), static(1), breathing(2), flow(3), flash(4),\n"
        "       successively(5), marquee(6), scanning(7), meteor(8)\n",
        program);
}

void args_print_version(void) {
    printf("leishen-led utility version 1.1.0\n");
}

int args_parse(int argc, char **argv, args_t *args) {
    if (!args) return -1;

    memset(args, 0, sizeof(*args));
    args->brightness = 70;
    args->r = 178;
    args->g = 0;
    args->b = 255;

    int opt;
    int option_index = 0;

    // 先解析全局选项
    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
        case 'h':
            args_print_usage(argv[0]);
            exit(0);
        case 'V':
            args_print_version();
            exit(0);
        case 'v':
            args->verbose = 1;
            break;
        case 'd':
            args->daemon = 1;
            break;
        case 256: // --mode
            {
                led_mode_t mode;
                if (led_parse_mode(optarg, &mode) != 0) {
                    fprintf(stderr, "Invalid mode: %s\n", optarg);
                    return -1;
                }
                args->mode = mode;
                args->has_mode = 1;
            }
            break;
        case 257: // --color
            if (optind + 2 > argc) {
                fprintf(stderr, "Color requires 3 arguments\n");
                return -1;
            }
            args->r = (uint8_t)strtol(argv[optind++], NULL, 0);
            args->g = (uint8_t)strtol(argv[optind++], NULL, 0);
            args->b = (uint8_t)strtol(argv[optind++], NULL, 0);
            args->has_color = 1;
            break;
        case 258: // --brightness
            args->brightness = (uint8_t)strtol(optarg, NULL, 0);
            args->has_brightness = 1;
            break;
        case 259: // --time
            args->time = (uint16_t)strtol(optarg, NULL, 0);
            args->has_time = 1;
            break;
        default:
            args_print_usage(argv[0]);
            return -1;
        }
    }

    return optind;
}

int args_validate(const args_t *args) {
    (void)args;
    return 0;
}

int args_to_state(const args_t *args, struct led_state *state) {
    if (!args || !state) return -1;

    // 读取当前状态或使用默认值
    if (led_read_state(state) != 0) {
        state->mode = LED_MODE_STATIC;
        state->brightness = 70;
        state->r = 178;
        state->g = 0;
        state->b = 255;
        state->time = 0;
    }

    // 根据参数更新
    if (args->has_mode) state->mode = args->mode;
    if (args->has_color) {
        state->r = args->r;
        state->g = args->g;
        state->b = args->b;
    }
    if (args->has_brightness) state->brightness = args->brightness;
    if (args->has_time) state->time = args->time;

    return 0;
}
