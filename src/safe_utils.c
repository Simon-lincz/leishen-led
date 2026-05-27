#include "safe_utils.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int safe_path(const char *base, const char *input, char *output, size_t out_size) {
    char combined[PATH_MAX];
    char resolved[PATH_MAX];
    int written = 0;

    // 清理输入：移除开头的 /
    while (*input == '/') input++;

    written = snprintf(combined, sizeof(combined), "%s/%s", base, input);
    if (written < 0 || (size_t)written >= sizeof(combined)) {
        return -1;
    }

    // 规范化路径
    if (realpath(combined, resolved) == NULL) {
        return -1;
    }

    // 安全检查：确保在 base 目录下
    char base_resolved[PATH_MAX];
    if (realpath(base, base_resolved) == NULL) {
        return -1;
    }

    size_t base_len = strlen(base_resolved);
    if (strncmp(resolved, base_resolved, base_len) != 0) {
        return -1;
    }

    // 确保分隔符或结束
    if (resolved[base_len] != '/' && resolved[base_len] != '\0') {
        return -1;
    }

    strncpy(output, resolved, out_size - 1);
    output[out_size - 1] = '\0';
    return 0;
}

int parse_http_request(const char *request, char *method, size_t msize,
                       char *path, size_t psize) {
    const char *p = request;

    // 跳过空白
    while (*p == ' ' || *p == '\t') p++;

    // 解析 method
    const char *method_end = p;
    while (*method_end && *method_end != ' ' && *method_end != '\t' &&
           *method_end != '\r' && *method_end != '\n') {
        method_end++;
    }

    size_t method_len = method_end - p;
    if (method_len == 0 || method_len >= msize) return -1;

    strncpy(method, p, method_len);
    method[method_len] = '\0';

    // 跳过空白
    p = method_end;
    while (*p == ' ' || *p == '\t') p++;

    // 解析 path
    const char *path_end = p;
    while (*path_end && *path_end != ' ' && *path_end != '\t' &&
           *path_end != '\r' && *path_end != '\n' && *path_end != '?') {
        path_end++;
    }

    size_t path_len = path_end - p;
    if (path_len == 0 || path_len >= psize) return -1;

    strncpy(path, p, path_len);
    path[path_len] = '\0';

    // 验证 method
    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0 &&
        strcmp(method, "HEAD") != 0 && strcmp(method, "OPTIONS") != 0 &&
        strcmp(method, "PUT") != 0 && strcmp(method, "DELETE") != 0) {
        return -1;
    }

    return 0;
}
