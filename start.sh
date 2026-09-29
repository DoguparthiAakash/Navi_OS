#!/bin/bash
# Start Navi OS in QEMU

cd "$(dirname "$0")" || exit 1

if [ ! -f NaviOS.iso ]; then
    echo "Error: NaviOS.iso not found. Please run ./build.sh first."
    exit 1
fi

echo "Starting Navi OS in QEMU..."
echo "  -vga std       : VBE 32bpp framebuffer for graphical mode"
echo "  -m 128M        : 128 MB RAM"
echo "  -no-reboot     : STOP on triple fault instead of boot-looping"
echo "  -d int,cpu_reset : Log interrupts and CPU resets to terminal"
echo ""

# Run QEMU — -no-reboot halts instead of looping on triple fault
qemu-system-i386 \
    -cdrom NaviOS.iso \
    -vga std \
    -m 128M \
    -no-reboot \
    2>&1 &

QEMU_PID=$!
echo $QEMU_PID > qemu.pid
echo "Navi OS is running in QEMU (PID: $QEMU_PID)."
