#ifndef SAFE_UTILS_H
#define SAFE_UTILS_H

#include <stddef.h>

// 安全的路径规范化，防止路径穿越
// 返回0成功，-1失败
int safe_path(const char *base, const char *input, char *output, size_t out_size);

// 安全的 HTTP 请求行解析，返回0成功
int parse_http_request(const char *request, char *method, size_t msize,
                       char *path, size_t psize);

#endif
