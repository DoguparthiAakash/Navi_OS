#!/bin/bash
# Navi OS Build Script - Pure Nux Edition
# NO Python. NO assembly files. Everything built by the Nux compiler.

set -e
cd "$(dirname "$0")" || exit 1

# ─── Colors ────────────────────────────────────────────────────────────────
BOLD='\033[1m'
CYAN='\033[96m'
GREEN='\033[92m'
DIM='\033[2m'
RESET='\033[0m'

echo -e "${BOLD}╭─ ◆ navi-os build ──────────────────────────────────────╮${RESET}"
echo -e "${BOLD}│  Navi OS  ·  pure Nux compilation                      │${RESET}"
echo -e "${BOLD}╰────────────────────────────────────────────────────────╯${RESET}"

# ─── Find the nux binary ───────────────────────────────────────────────────
# We need a Linux (ELF) build of nux since this runs inside WSL.
NUX_SRC="$(dirname "$0")/../Nux_Lang/nux/nux_oleg/nux_dist"
NUX_BIN_PATH="$NUX_SRC/target/release/nux"

if [ ! -f "$NUX_BIN_PATH" ]; then
    echo -e "${DIM}  ├─ · Building Nux compiler from source...${RESET}"
    (cd "$NUX_SRC" && PATH="$HOME/.cargo/bin:$PATH" cargo build --release -q)
fi

NUX_BIN="$NUX_BIN_PATH"
if [ ! -f "$NUX_BIN" ]; then
    echo -e "  ╰─ ✕ ${BOLD}Cannot find or build the nux compiler. Is cargo/rustup installed?${RESET}"
    exit 1
fi

echo -e "${DIM}  ├─ · Using Nux compiler: $NUX_BIN${RESET}"

# ─── Step 1: Compile the Nux OS source files into boot.o + kernel_nux.o ───
echo -e "${DIM}  ├─ ✦ Compiling boot.nux → boot.o${RESET}"
"$NUX_BIN" build-native boot.nux --output boot.o

echo -e "${DIM}  ├─ ✦ Compiling kernel → kernel_nux.o${RESET}"
"$NUX_BIN" build-native \
    fs.nux \
    ../Nux_Lang/lib/std/hw.nux \
    ../Nux_Lang/lib/std/mem.nux \
    ../Nux_Lang/lib/std/string.nux \
    ../Nux_Lang/lib/std/io.nux \
    ../Nux_Lang/lib/std/math.nux \
    keyboard.nux \
    ramfs.nux \
    edit.nux \
    fm.nux \
    shell.nux \
    kernel.nux \
    --output kernel_nux.o

# ─── Step 2: Link ─────────────────────────────────────────────────────────
echo -e "${DIM}  ├─ · Linking Navi OS...${RESET}"
ld -m elf_i386 -T linker.ld boot.o kernel_nux.o -o navi.bin -nostdlib

# ─── Step 3: Verify Multiboot ─────────────────────────────────────────────
echo -e "${DIM}  ├─ · Verifying Multiboot header...${RESET}"
if grub-file --is-x86-multiboot navi.bin; then
  echo -e "${DIM}  ├─ ✦ Multiboot confirmed.${RESET}"
else
  echo -e "  ╰─ ✕ ${BOLD}Error: navi.bin is not a valid Multiboot image.${RESET}"
  exit 1
fi

# ─── Step 4: Generate RamFS ───────────────────────────────────────────────
echo -e "${DIM}  ├─ · Generating NuxFS Ramdisk...${RESET}"
mkdir -p ramfs_root
echo "Welcome to Navi OS! Powered by the Nux language." > ramfs_root/readme.txt
touch initrd.img

# ─── Step 5: Package ISO ──────────────────────────────────────────────────
echo -e "${DIM}  ├─ · Packaging NaviOS.iso...${RESET}"
mkdir -p isodir/boot/grub
cp navi.bin isodir/boot/navi.bin
cp initrd.img isodir/boot/initrd.img
cp grub.cfg isodir/boot/grub/grub.cfg

grub-mkrescue -o NaviOS.iso isodir 2>/dev/null

echo ""
echo -e "${GREEN}${BOLD}  ╰─ ✦ Build successful!  NaviOS.iso${RESET}"
echo -e "${DIM}     Run:  qemu-system-i386 -cdrom NaviOS.iso${RESET}"
echo -e "${DIM}     Or load NaviOS.iso into VirtualBox.${RESET}"
echo ""
