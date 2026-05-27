#ifndef LEISHEN_API_H
#define LEISHEN_API_H

#include <stddef.h>

struct led_state;

// API 处理函数类型
typedef void (*api_handler_t)(int fd, const char *method, const char *body, void *ctx);

struct api_dispatch_ctx {
    const char *state_path;
    int (*schedule_save)(const char *path, const struct led_state *state);
};

// API 路由条目
typedef struct {
    const char *method;
    const char *path;
    api_handler_t handler;
} api_route_t;

// 初始化 API 模块
void api_init(void);

// 路由分发
void api_dispatch(int fd, const char *method, const char *path,
                  const char *body, void *ctx);

// 获取可用路由列表（用于 OPTIONS 预检）
const api_route_t *api_get_routes(size_t *count);

// 通用响应函数
void api_send_json(int fd, int status, const char *status_text,
                    const char *json, size_t json_len);
void api_send_error(int fd, int status, const char *message);
void api_send_cors(int fd);

void handle_status(int fd, const char *method, const char *body, void *ctx);
void handle_presets(int fd, const char *method, const char *body, void *ctx);
void handle_apply(int fd, const char *method, const char *body, void *ctx);
void handle_effect(int fd, const char *method, const char *body, void *ctx);
void handle_off(int fd, const char *method, const char *body, void *ctx);
void handle_modes(int fd, const char *method, const char *body, void *ctx);

#endif
