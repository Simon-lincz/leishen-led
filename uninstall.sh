#!/usr/bin/env bash
set -euo pipefail

PREFIX="/opt/leishen-led"
STATE_DIR="/var/lib/leishen-led"
SERVICE_FILE="/etc/systemd/system/leishen-led.service"

remove_firewall_rules() {
  if command -v ufw >/dev/null 2>&1; then
    ufw --force delete allow from 100.64.0.0/10 to any port 8787 proto tcp >/dev/null 2>&1 || true
    ufw --force delete allow from 192.168.0.0/16 to any port 8787 proto tcp >/dev/null 2>&1 || true
    ufw --force delete deny from any to any port 8787 proto tcp >/dev/null 2>&1 || true
  fi
}

if [[ "${EUID}" -ne 0 ]]; then
  echo "Please run with sudo: sudo ./uninstall.sh" >&2
  exit 1
fi

systemctl disable --now leishen-led.service >/dev/null 2>&1 || true
remove_firewall_rules
rm -f "${SERVICE_FILE}"
systemctl daemon-reload

rm -rf "${PREFIX}"

if [[ "${1:-}" == "--purge" ]]; then
  rm -rf "${STATE_DIR}"
  echo "Uninstalled and removed saved LED state."
else
  echo "Uninstalled. Saved LED state kept at ${STATE_DIR}."
  echo "Run sudo ./uninstall.sh --purge to remove saved state too."
fi
