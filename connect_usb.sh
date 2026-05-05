#!/usr/bin/env bash

set -e

KINDLE_IP="192.168.15.244"
LOCAL_IP="192.168.15.201/24"

echo "[Leafy] Searching for Kindle USB interface..."

IFACE=$(ip -o link show | awk -F': ' '
    $2 ~ /^enx/ ||
    $2 ~ /^usb/ ||
    $2 ~ /^rndis/ ||
    $2 ~ /^enp.*usb/ ||
    $2 ~ /^enp.*f0u/ {print $2}
' | head -n 1)

# fallback: match by IP pattern (VERY useful for Kindles)
if [ -z "$IFACE" ]; then
    IFACE=$(ip -o addr show | awk '
        /169\.254\./ {print $2; exit}
    ')
fi

if [ -z "$IFACE" ]; then
    echo "[Leafy] ERROR: No Kindle USB interface found."
    exit 1
fi

echo "[Leafy] Found interface: $IFACE"

echo "[Leafy] Bringing interface up..."
sudo ip link set "$IFACE" up

echo "[Leafy] Setting IP $LOCAL_IP ..."
sudo ip addr flush dev "$IFACE" 2>/dev/null || true
sudo ip addr add "$LOCAL_IP" dev "$IFACE"

echo "[Leafy] Waiting for Kindle..."

for i in {1..10}; do
    if ping -c 1 -W 1 "$KINDLE_IP" >/dev/null 2>&1; then
        echo "[Leafy] Kindle reachable!"
        break
    fi
    sleep 1
done

echo "[Leafy] SSH into Kindle..."
ssh root@"$KINDLE_IP"