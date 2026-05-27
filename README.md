# Leishen MIX II PRO 迷你PC:LED Linux 控制服务

这个项目把雷神 T-Control 的灯效控制移植成 Linux 本地服务。服务监听 `0.0.0.0:8787`，但只允许 `127.0.0.0/8`、`100.64.0.0/10`、`192.168.0.0/16` 访问。

## 硬件接口

EC 端口：

```text
data: 0x62
cmd:  0x66
```

灯效白名单 offset：

```text
0x95 mode
0x98 brightness
0x9A red
0x9B green
0x9C blue
0x9D time high
0x9E time low
```

模式：

```text
0 off
1 static
2 breathing
3 flow
4 flash
5 successively
6 marquee
7 scanning
8 meteor
```

## 安装

```bash
sudo ./install.sh
```

安装内容：

```text
/opt/leishen-led/bin/leishen_led
/opt/leishen-led/bin/leishen-ledd
/opt/leishen-led/web/*
/etc/systemd/system/leishen-led.service
/var/lib/leishen-led/state.json
```

服务会自动启动并设置开机自启：

```bash
systemctl status leishen-led.service
```

打开控制台：

```text
http://127.0.0.1:8787/
http://192.168.x.x:8787/
http://100.64.x.x:8787/
```

安装脚本会在存在 UFW 时自动添加：

```bash
ufw allow from 100.64.0.0/10 to any port 8787 proto tcp
ufw allow from 192.168.0.0/16 to any port 8787 proto tcp
ufw deny from any to any port 8787 proto tcp
```

服务自身也会检查来源 IP，不在白名单内会返回 `403`。

## 卸载

保留上次灯效配置：

```bash
sudo ./uninstall.sh
```

彻底删除：

```bash
sudo ./uninstall.sh --purge
```

## 命令行工具

```bash
sudo /opt/leishen-led/bin/leishen_led status
sudo /opt/leishen-led/bin/leishen_led mode flow
sudo /opt/leishen-led/bin/leishen_led color 178 0 255
sudo /opt/leishen-led/bin/leishen_led brightness 70
sudo /opt/leishen-led/bin/leishen_led time 0
sudo /opt/leishen-led/bin/leishen_led set --mode static --color 178 0 255 --brightness 70 --time 0
```

## API

```text
GET  /api/status
GET  /api/presets
GET  /api/modes
POST /api/apply
POST /api/effect
POST /api/off
```

`POST /api/apply` 请求：

```json
{
  "mode": "static",
  "brightness": 70,
  "color": [178, 0, 255],
  "time": 0
}
```

每次成功写入后，服务会保存到 `/var/lib/leishen-led/state.json`，下次开机自动恢复。
