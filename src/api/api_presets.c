#include "api.h"

#include <string.h>

// 内置预设列表
static const char *built_in_presets =
    "["
    "  {\"name\":\"off\",\"label\":\"关闭\",\"mode\":\"off\",\"color\":[0,0,0],\"brightness\":0},"
    "  {\"name\":\"red\",\"label\":\"红色\",\"mode\":\"static\",\"color\":[255,0,0],\"brightness\":100},"
    "  {\"name\":\"green\",\"label\":\"绿色\",\"mode\":\"static\",\"color\":[0,255,0],\"brightness\":100},"
    "  {\"name\":\"blue\",\"label\":\"蓝色\",\"mode\":\"static\",\"color\":[0,0,255],\"brightness\":100},"
    "  {\"name\":\"purple\",\"label\":\"紫色呼吸\",\"mode\":\"breathing\",\"color\":[178,0,255],\"brightness\":70,\"time\":5},"
    "  {\"name\":\"cyan\",\"label\":\"青色流光\",\"mode\":\"flow\",\"color\":[0,255,255],\"brightness\":80,\"time\":3},"
    "  {\"name\":\"rainbow\",\"label\":\"彩虹\",\"mode\":\"flow\",\"color\":[255,128,0],\"brightness\":100,\"time\":2},"
    "  {\"name\":\"meteor_blue\",\"label\":\"蓝色流星\",\"mode\":\"meteor\",\"color\":[0,150,255],\"brightness\":90,\"time\":4}"
    "]";

void handle_presets(int fd, const char *method, const char *body, void *ctx) {
    (void)method; (void)body; (void)ctx;
    api_send_json(fd, 200, "OK", built_in_presets, strlen(built_in_presets));
}
