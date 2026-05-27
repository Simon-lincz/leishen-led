#define _GNU_SOURCE

#include "api/api.h"
#include "ec.h"
#include "led_protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8787
#define DEFAULT_WEB_ROOT "/opt/leishen-led/web"
#define DEFAULT_STATE_PATH "/var/lib/leishen-led/state.json"
#define REQ_MAX 32768

static volatile sig_atomic_t running = 1;

struct config {
    char host[64];
    int port;
    char web_root[256];
    char state_path[256];
};

static void on_signal(int signo) {
    (void)signo;
    running = 0;
}

static void default_config(struct config *config) {
    snprintf(config->host, sizeof(config->host), "%s", DEFAULT_HOST);
    config->port = DEFAULT_PORT;
    snprintf(config->web_root, sizeof(config->web_root), "%s", DEFAULT_WEB_ROOT);
    snprintf(config->state_path, sizeof(config->state_path), "%s", DEFAULT_STATE_PATH);
}

static int parse_listen(const char *text, struct config *config) {
    const char *colon = strrchr(text, ':');
    char port_text[16];
    long port = 0;
    char *end = NULL;

    if (!colon || colon == text || strlen(colon + 1) >= sizeof(port_text)) {
        return -1;
    }

    snprintf(config->host, sizeof(config->host), "%.*s", (int)(colon - text), text);
    snprintf(port_text, sizeof(port_text), "%s", colon + 1);
    port = strtol(port_text, &end, 10);

    if (*port_text == '\0' || *end != '\0' || port < 1 || port > 65535) {
        return -1;
    }

    config->port = (int)port;
    return 0;
}

static void print_usage(const char *program) {
    fprintf(stderr,
        "Usage: %s [--listen 127.0.0.1:8787] [--web-root DIR] [--state PATH]\n",
        program);
}

static int parse_args(int argc, char **argv, struct config *config) {
    default_config(config);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--listen") == 0) {
            if (++i >= argc || parse_listen(argv[i], config)) {
                return -1;
            }
        } else if (strcmp(argv[i], "--web-root") == 0) {
            if (++i >= argc) {
                return -1;
            }
            snprintf(config->web_root, sizeof(config->web_root), "%s", argv[i]);
        } else if (strcmp(argv[i], "--state") == 0) {
            if (++i >= argc) {
                return -1;
            }
            snprintf(config->state_path, sizeof(config->state_path), "%s", argv[i]);
        } else {
            return -1;
        }
    }

    return 0;
}

static const char *content_type(const char *path) {
    const char *ext = strrchr(path, '.');

    if (!ext) {
        return "application/octet-stream";
    }
    if (strcmp(ext, ".html") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcmp(ext, ".css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcmp(ext, ".js") == 0) {
        return "application/javascript; charset=utf-8";
    }
    if (strcmp(ext, ".json") == 0) {
        return "application/json; charset=utf-8";
    }

    return "application/octet-stream";
}

static void send_response(int fd, int status, const char *status_text,
                          const char *type, const char *body) {
    size_t length = body ? strlen(body) : 0;
    size_t written = 0;
    dprintf(fd,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n",
        status, status_text, type, length);
    while (written < length) {
        ssize_t result = write(fd, body + written, length - written);
        if (result <= 0) {
            break;
        }
        written += (size_t)result;
    }
}

static bool is_allowed_client(struct sockaddr_in *peer) {
    uint32_t ip = ntohl(peer->sin_addr.s_addr);

    if ((ip & 0xff000000u) == 0x7f000000u) {
        return true;
    }
    if ((ip & 0xffc00000u) == 0x64400000u) {
        return true;
    }
    if ((ip & 0xffff0000u) == 0xc0a80000u) {
        return true;
    }

    return false;
}

static int save_state(const char *path, const struct led_state *state) {
    FILE *file = fopen(path, "w");

    if (!file) {
        return -1;
    }

    fprintf(file,
        "{\n"
        "  \"mode\": \"%s\",\n"
        "  \"brightness\": %u,\n"
        "  \"color\": [%u, %u, %u],\n"
        "  \"time\": %u\n"
        "}\n",
        led_mode_label(state->mode), state->brightness,
        state->r, state->g, state->b, state->time);
    fclose(file);
    return 0;
}

static char *read_file_text(const char *path, size_t max_size) {
    FILE *file = fopen(path, "rb");
    long size = 0;
    char *buffer = NULL;

    if (!file) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 ||
        (size_t)size > max_size || fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return NULL;
    }

    buffer = calloc(1, (size_t)size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return buffer;
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

static int parse_state_json(const char *json, struct led_state *state) {
    char mode_text[32];
    int value = 0;

    if (json_find_string(json, "mode", mode_text, sizeof(mode_text)) == 0) {
        if (led_parse_mode(mode_text, &state->mode)) {
            return -1;
        }
    } else if (json_find_int(json, "mode", 0, 8, &value) == 0) {
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

static int load_state(const char *path, struct led_state *state) {
    char *json = read_file_text(path, 4096);
    int result = -1;

    if (!json) {
        return -1;
    }

    result = parse_state_json(json, state);
    free(json);
    return result;
}

static void serve_file(int fd, const struct config *config, const char *url_path) {
    char path[512];
    char *body = NULL;

    if (strstr(url_path, "..")) {
        send_response(fd, 400, "Bad Request", "text/plain; charset=utf-8", "bad path\n");
        return;
    }

    if (strcmp(url_path, "/") == 0) {
        url_path = "/index.html";
    }

    snprintf(path, sizeof(path), "%s%s", config->web_root, url_path);
    body = read_file_text(path, 1024 * 1024);
    if (!body) {
        send_response(fd, 404, "Not Found", "text/plain; charset=utf-8", "not found\n");
        return;
    }

    send_response(fd, 200, "OK", content_type(path), body);
    free(body);
}

static void handle_api(int fd, const struct config *config, const char *method,
                       const char *path, const char *body) {
    struct api_dispatch_ctx ctx = {
        .state_path = config->state_path,
        .schedule_save = save_state,
    };

    api_dispatch(fd, method, path, body, &ctx);
}

static void handle_client(int fd, const struct config *config) {
    char request[REQ_MAX + 1];
    ssize_t read_len = read(fd, request, REQ_MAX);
    char method[8];
    char path[256];
    char *header_end = NULL;
    char *body = "";

    if (read_len <= 0) {
        return;
    }

    request[read_len] = '\0';
    if (sscanf(request, "%7s %255s", method, path) != 2) {
        send_response(fd, 400, "Bad Request", "text/plain; charset=utf-8", "bad request\n");
        return;
    }

    header_end = strstr(request, "\r\n\r\n");
    if (header_end) {
        body = header_end + 4;
    }

    if (strncmp(path, "/api/", 5) == 0) {
        handle_api(fd, config, method, path, body);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        send_response(fd, 405, "Method Not Allowed", "text/plain; charset=utf-8", "method not allowed\n");
        return;
    }

    serve_file(fd, config, path);
}

static int create_server(const struct config *config) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int enabled = 1;
    struct sockaddr_in addr;

    if (fd < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)config->port);
    if (inet_pton(AF_INET, config->host, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid listen host: %s\n", config->host);
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 16) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    struct config config;
    struct led_state saved;
    int server_fd = -1;

    if (parse_args(argc, argv, &config)) {
        print_usage(argv[0]);
        return 1;
    }

    if (ec_init()) {
        return 1;
    }

    if (load_state(config.state_path, &saved) == 0) {
        if (led_apply_state(&saved)) {
            fprintf(stderr, "warning: failed to restore saved LED state\n");
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    server_fd = create_server(&config);
    if (server_fd < 0) {
        return 1;
    }

    fprintf(stderr, "leishen-ledd listening on http://%s:%d/\n", config.host, config.port);

    while (running) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int client_fd = accept(server_fd, (struct sockaddr *)&peer, &peer_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            break;
        }

        if (!is_allowed_client(&peer)) {
            send_response(client_fd, 403, "Forbidden", "application/json; charset=utf-8",
                "{\"error\":\"client network is not allowed\"}");
            close(client_fd);
            continue;
        }

        handle_client(client_fd, &config);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
