#include "api.h"
#include "../led_protocol.h"
#include "../logger.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 路由表
static const api_route_t routes[] = {
    // GET 路由
    {"GET",  "/api/status",   handle_status},
    {"GET",  "/api/presets", handle_presets},
    {"GET",  "/api/modes",   handle_modes},
    // POST 路由
    {"POST", "/api/apply",   handle_apply},
    {"POST", "/api/effect",  handle_effect},
    {"POST", "/api/off",     handle_off},
    // 终止标记
    {NULL, NULL, NULL}
};

static void write_all(int fd, const char *buffer, size_t length) {
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(fd, buffer + written, length - written);
        if (result <= 0) {
            return;
        }
        written += (size_t)result;
    }
}

void api_init(void) {
    // 当前无需初始化，后续可扩展配置加载
}

const api_route_t *api_get_routes(size_t *count) {
    if (count) {
        *count = (sizeof(routes) / sizeof(routes[0])) - 1;
    }
    return routes;
}

void api_dispatch(int fd, const char *method, const char *path,
                  const char *body, void *ctx) {
    LLOG_DEBUG("API request: %s %s", method, path);

    // 处理 OPTIONS 预检请求
    if (strcmp(method, "OPTIONS") == 0) {
        api_send_cors(fd);
        return;
    }

    // 查找路由
    for (int i = 0; routes[i].path != NULL; i++) {
        if (strcmp(method, routes[i].method) == 0 &&
            strcmp(path, routes[i].path) == 0) {
            routes[i].handler(fd, method, body, ctx);
            return;
        }
    }

    // 未找到路由
    api_send_error(fd, 404, "not found");
}

void api_send_json(int fd, int status, const char *status_text,
                    const char *json, size_t json_len) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n",
        status, status_text, json_len);

    if (header_len < 0) {
        return;
    }
    write_all(fd, header, (size_t)header_len);
    if (json && json_len > 0) {
        write_all(fd, json, json_len);
    }
}

void api_send_error(int fd, int status, const char *message) {
    char body[256];
    int len = snprintf(body, sizeof(body), "{\"error\":\"%s\"}", message);
    api_send_json(fd, status,
                  status == 404 ? "Not Found" :
                  status == 400 ? "Bad Request" : "Error",
                  body, len);
}

void api_send_cors(int fd) {
    const char *response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Access-Control-Max-Age: 86400\r\n"
        "Connection: close\r\n"
        "\r\n";
    write_all(fd, response, strlen(response));
}
