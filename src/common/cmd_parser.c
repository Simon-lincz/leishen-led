#include "common.h"
#include "../led_protocol.h"

#include <stdio.h>
#include <string.h>

// 子命令信息表
static const cmd_info_t commands[] = {
    {"status",      NULL,   CMD_STATUS,      NULL,            "显示当前灯效状态"},
    {"mode",        NULL,   CMD_MODE,        "<mode>",        "设置灯效模式"},
    {"color",       NULL,   CMD_COLOR,       "<r> <g> <b>",    "设置颜色 (0-255)"},
    {"brightness",  "bri",  CMD_BRIGHTNESS,  "<0-100>",        "设置亮度"},
    {"bri",         NULL,   CMD_BRIGHTNESS,  "<0-100>",        "设置亮度 (简写)"},
    {"time",        NULL,   CMD_TIME,        "<0-65535>",      "设置时间参数"},
    {"set",         NULL,   CMD_SET,         "[options]",      "批量设置所有参数"},
    {"help",        "?",    CMD_HELP,        NULL,             "显示帮助"},
    {"version",     "v",    CMD_VERSION,     NULL,             "显示版本"},
};

cmd_type_t cmd_parse(const char *name) {
    if (!name) return CMD_UNKNOWN;

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(name, commands[i].name) == 0) {
            return commands[i].type;
        }
        if (commands[i].alias && strcmp(name, commands[i].alias) == 0) {
            return commands[i].type;
        }
    }
    return CMD_UNKNOWN;
}

const cmd_info_t *cmd_get_info(cmd_type_t type) {
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (commands[i].type == type) {
            return &commands[i];
        }
    }
    return NULL;
}

const cmd_info_t *cmd_find(const char *name) {
    if (!name) return NULL;

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(name, commands[i].name) == 0 ||
            (commands[i].alias && strcmp(name, commands[i].alias) == 0)) {
            return &commands[i];
        }
    }
    return NULL;
}

void cmd_print_all_help(const char *program) {
    fprintf(stderr, "Usage: %s <command> [args...]\n\n", program);
    fprintf(stderr, "Commands:\n");

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        const cmd_info_t *cmd = &commands[i];
        fprintf(stderr, "  %-12s", cmd->name);
        if (cmd->alias) {
            fprintf(stderr, " (%s)", cmd->alias);
        }
        fprintf(stderr, "  %s\n", cmd->description);
        if (cmd->usage) {
            fprintf(stderr, "%*s usage: %s %s\n", 16, "", program, cmd->usage);
        }
    }

    fprintf(stderr, "\nModes: off, static, breathing, flow, flash, successively, marquee, scanning, meteor\n");
    fprintf(stderr, "       or numeric 0-8\n");
}
