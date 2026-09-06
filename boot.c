#include <stdint.h>
#include <stdbool.h>
static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
// boot.nux - Multiboot header and kernel entry point
// Written entirely in Nux - no assembly files needed.
// The Nux compiler's native backend handles the inline asm blocks.


// Multiboot magic constants
uint32_t MULTIBOOT_MAGIC = 0x1BADB002;
uint32_t MULTIBOOT_FLAGS = 0x00000003;    // ALIGN | MEMINFO
uint32_t MULTIBOOT_CHECKSUM = -(0x1BADB002 + 0x00000003);

// The Nux native compiler emits this as a .multiboot section with .long directives.
// GCC + the linker script (linker.ld) will place this at the correct location.
__asm__(".section .multiboot\n"
    ".align 4\n"
    ".long 0x1BADB002\n"
    ".long 0x00000003\n"
    ".long -(0x1BADB002 + 0x00000003)\n");

// 16 KiB stack in BSS
__asm__(".section .bss\n"
    ".align 16\n"
    "stack_bottom:\n"
    ".skip 16384\n"
    "stack_top:\n");

// _start: the entry point called by GRUB

void boot_entry() {
    __asm__(".section .text\n"
        ".global _start\n"
        ".type _start, @function\n"
        "_start:\n"
        "    mov $stack_top, %esp\n"
        "    call kmain\n"
        "    cli\n"
        "1:  hlt\n"
        "    jmp 1b\n"
        ".size _start, . - _start\n");
}

