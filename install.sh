#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="/opt/leishen-led"
STATE_DIR="/var/lib/leishen-led"
SERVICE_FILE="/etc/systemd/system/leishen-led.service"

configure_firewall() {
  if command -v ufw >/dev/null 2>&1; then
    ufw --force delete allow from 100.64.0.0/10 to any port 8787 proto tcp >/dev/null 2>&1 || true
    ufw --force delete allow from 192.168.0.0/16 to any port 8787 proto tcp >/dev/null 2>&1 || true
    ufw --force delete deny from any to any port 8787 proto tcp >/dev/null 2>&1 || true
    ufw allow from 100.64.0.0/10 to any port 8787 proto tcp comment "leishen-led cgnat" >/dev/null
    ufw allow from 192.168.0.0/16 to any port 8787 proto tcp comment "leishen-led lan" >/dev/null
    ufw deny from any to any port 8787 proto tcp comment "leishen-led default deny" >/dev/null
    echo "UFW rules installed for 100.64.0.0/10 and 192.168.0.0/16."
    if ! ufw status | grep -q "Status: active"; then
      echo "UFW is installed but not active. Enable it with: sudo ufw enable"
    fi
  else
    echo "UFW not found. Service still filters clients internally."
    echo "If you use another firewall, allow TCP 8787 from 100.64.0.0/10 and 192.168.0.0/16 only."
  fi
}

if [[ "${EUID}" -ne 0 ]]; then
  echo "Please run with sudo: sudo ./install.sh" >&2
  exit 1
fi

if ! command -v gcc >/dev/null 2>&1; then
  echo "gcc is required. Install build-essential first." >&2
  exit 1
fi

if ! command -v make >/dev/null 2>&1; then
  echo "make is required. Install build-essential first." >&2
  exit 1
fi

make -C "${PROJECT_DIR}" all

install -d -m 0755 "${PREFIX}/bin"
install -d -m 0755 "${PREFIX}/web"
install -d -m 0755 "${STATE_DIR}"

install -m 0755 "${PROJECT_DIR}/build/leishen_led" "${PREFIX}/bin/leishen_led"
install -m 0755 "${PROJECT_DIR}/build/leishen-ledd" "${PREFIX}/bin/leishen-ledd"
install -m 0644 "${PROJECT_DIR}/web/index.html" "${PREFIX}/web/index.html"
install -m 0644 "${PROJECT_DIR}/web/style.css" "${PREFIX}/web/style.css"
install -m 0644 "${PROJECT_DIR}/web/app.js" "${PREFIX}/web/app.js"

if [[ ! -f "${STATE_DIR}/state.json" ]]; then
  install -m 0644 /dev/stdin "${STATE_DIR}/state.json" <<'JSON'
{
  "mode": "static",
  "brightness": 70,
  "color": [178, 0, 255],
  "time": 0
}
JSON
fi

install -m 0644 "${PROJECT_DIR}/systemd/leishen-led.service" "${SERVICE_FILE}"

configure_firewall

systemctl daemon-reload
systemctl enable leishen-led.service
systemctl restart --no-block leishen-led.service

echo "Installed."
echo "Open: http://127.0.0.1:8787/"
echo "LAN:  http://<192.168.x.x>:8787/"
echo "CGN:  http://<100.64.x.x>:8787/"
echo "Status: systemctl status leishen-led.service"
