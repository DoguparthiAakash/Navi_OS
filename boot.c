#include <stdint.h>
#include <stdbool.h>
static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
// boot.nux - Multiboot header and kernel entry point
// Written entirely in Nux - no assembly files needed.
// The Nux compiler's native backend handles the inline asm blocks.


// Multiboot magic constants
// FLAGS bits: bit0=page-align modules, bit1=include memory map, bit2=set video mode
// Setting bit2 lets us request a specific video mode via header fields.
uint32_t MULTIBOOT_MAGIC = 0x1BADB002;
uint32_t MULTIBOOT_FLAGS = 0x00000007;  // ALIGN | MEMINFO | VIDEO_MODE
uint32_t MULTIBOOT_CHECKSUM = -(0x1BADB002 + 0x00000007);

// Emit the Multiboot header with video mode fields (type=0 = linear framebuffer).
// Fields: mode_type, width, height, depth (0 = no preference for width/height/depth).
__asm__(".section .multiboot\n"
    ".align 4\n"
    ".long 0x1BADB002\n"               // magic
    ".long 0x00000007\n"               // flags (ALIGN|MEMINFO|VIDEO_MODE)
    ".long -(0x1BADB002+0x00000007)\n" // checksum
    ".long 0\n"                        // header_addr   (not used for flags < 0x10000)
    ".long 0\n"                        // load_addr
    ".long 0\n"                        // load_end_addr
    ".long 0\n"                        // bss_end_addr
    ".long 0\n"                        // entry_addr
    ".long 0\n"                        // mode_type: 0=linear framebuffer
    ".long 0\n"                        // width:  0=no preference
    ".long 0\n"                        // height: 0=no preference
    ".long 32\n");                     // depth:  32bpp

// 16 KiB stack in BSS
__asm__(".section .bss\n"
    ".align 16\n"
    "stack_bottom:\n"
    ".skip 16384\n"
    "stack_top:\n");

// _start: the entry point called by GRUB.
// EAX = 0x2BADB002 (Multiboot magic, we ignore it)
// EBX = physical address of the Multiboot information structure (MBI)
// We push EBX so kmain receives it as its first C argument.

void boot_entry() {
    __asm__(".section .text\n"
        ".global _start\n"
        ".type _start, @function\n"
        "_start:\n"
        "    mov $stack_top, %esp\n"   // set up kernel stack
        "    push %ebx\n"             // arg0 = MBI pointer (from GRUB)
        "    push %eax\n"             // arg1 = Multiboot magic (unused)
        "    call kmain\n"
        "    cli\n"
        "1:  hlt\n"
        "    jmp 1b\n"
        ".size _start, . - _start\n");
}


