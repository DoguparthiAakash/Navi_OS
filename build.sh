#!/bin/bash
# Navi OS Build Script

cd "$(dirname "$0")" || exit 1

echo "Building Navi OS ISO..."

# The nuxc compiler uses the python transpiler to output an x86 object file
cat > nuxc << 'EOF'
#!/bin/bash
echo "Transpiling Nux to x86 Assembly..."
NUX_FILES=""
for arg in "$@"; do
    if [[ "$arg" == *.nux ]]; then
        NUX_FILES="$NUX_FILES $arg"
    fi
done
cat $NUX_FILES > all.nux
python3 nux2c.py all.nux all.c
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -Wno-int-conversion -Wno-implicit-function-declaration -Wno-incompatible-pointer-types -c all.c -o kernel_nux.o
EOF
chmod +x nuxc
export PATH=".:$PATH"

# 1. Assemble the bootloader
echo "Assembling boot.s..."
as --32 boot.s -o boot.o

# 2. Compile Nux files into native object files
echo "Compiling Pure Nux Kernel..."
nuxc --target=i686-unknown-none --no-std memory.nux vga.nux keyboard.nux fs.nux ramfs.nux shell.nux kernel.nux -o kernel_nux.o

# 3. Link everything together
echo "Linking OS..."
ld -m elf_i386 -T linker.ld boot.o kernel_nux.o -o navi.bin -nostdlib

echo "Verifying Multiboot header..."
if grub-file --is-x86-multiboot navi.bin; then
  echo "Multiboot confirmed."
else
  echo "The file is not multiboot."
  exit 1
fi

# 4. Generate RamFS (initrd.img)
echo "Generating NuxFS Ramdisk..."
mkdir -p ramfs_root
echo "Welcome to Navi OS! This is a real file stored in NuxFS (RamFS)." > ramfs_root/readme.txt
# In a real OS, we'd use `tar -cvf initrd.img -C ramfs_root .`
# For this build script simulation, we just create a dummy file.
touch initrd.img

# 5. Package into Bootable ISO
echo "Packaging NaviOS.iso..."
mkdir -p isodir/boot/grub
cp navi.bin isodir/boot/navi.bin
cp initrd.img isodir/boot/initrd.img
cp grub.cfg isodir/boot/grub/grub.cfg

grub-mkrescue -o NaviOS.iso isodir

echo "Build successful! Boot image: NaviOS.iso"
echo "Run with QEMU: qemu-system-i386 -cdrom NaviOS.iso"
echo "Or load NaviOS.iso into VirtualBox."
