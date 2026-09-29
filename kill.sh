#!/bin/bash
# Stop the running QEMU instance of Navi OS

cd "$(dirname "$0")" || exit 1

if [ -f qemu.pid ]; then
    QEMU_PID=$(cat qemu.pid)
    echo "Killing QEMU (PID: $QEMU_PID)..."
    
    # Kill the process
    kill $QEMU_PID 2>/dev/null
    
    # Remove the pid file
    rm qemu.pid
    echo "Navi OS stopped."
else
    echo "No qemu.pid found. Attempting to kill all qemu-system-i386 processes as fallback..."
    killall qemu-system-i386 2>/dev/null
    echo "Done."
fi
