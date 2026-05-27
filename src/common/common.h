#ifndef LEISHEN_COMMON_H
#define LEISHEN_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// 前向声明 led_state（避免循环依赖）
struct led_state;

// ============== 子命令定义 ==============

// 子命令类型
typedef enum {
    CMD_UNKNOWN = -1,
    CMD_STATUS,
    CMD_MODE,
    CMD_COLOR,
    CMD_BRIGHTNESS,
    CMD_TIME,
    CMD_SET,
    CMD_HELP,
    CMD_VERSION
} cmd_type_t;

// 子命令结构
typedef struct {
    const char *name;
    const char *alias;
    cmd_type_t type;
    const char *usage;
    const char *description;
} cmd_info_t;

// 查找子命令
cmd_type_t cmd_parse(const char *name);
const cmd_info_t *cmd_get_info(cmd_type_t type);
const cmd_info_t *cmd_find(const char *name);
void cmd_print_all_help(const char *program);

// ============== 参数解析 ==============

// 解析结果
typedef struct {
    // LED 参数
    int has_mode;
    uint8_t mode;
    int has_color;
    uint8_t r, g, b;
    int has_brightness;
    uint8_t brightness;
    int has_time;
    uint16_t time;
    // 其他
    int verbose;
    int daemon;
} args_t;

// 解析参数（支持 getopt 风格）
int args_parse(int argc, char **argv, args_t *args);

// 合并参数到 led_state
int args_to_state(const args_t *args, struct led_state *state);

// 验证参数合法性
int args_validate(const args_t *args);

// 打印用法
void args_print_usage(const char *program);

// 打印版本
void args_print_version(void);

#endif
