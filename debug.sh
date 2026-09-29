#!/bin/bash
# Debug Navi OS in QEMU — shows interrupt log in terminal.
# If a triple fault happens, QEMU stops (no reboot) and prints what CPU was doing.

cd "$(dirname "$0")" || exit 1

if [ ! -f NaviOS.iso ]; then
    echo "Error: NaviOS.iso not found. Please run ./build.sh first."
    exit 1
fi

echo "Starting Navi OS in DEBUG mode..."

qemu-system-i386 \
    -cdrom NaviOS.iso \
    -vga std \
    -m 128M \
    -no-reboot \
    -d int,cpu_reset 2>&1 | tee qemu_debug.log
