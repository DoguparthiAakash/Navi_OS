/* boot.s - Multiboot header and kernel entry point */

/* Constants for multiboot header */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* Declare the multiboot header in its own section so linker can place it at the front */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Provide a stack for the OS */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB stack
stack_top:

/* The entry point called by GRUB */
.section .text
.global _start
.type _start, @function
_start:
    /* Set up the stack */
    mov $stack_top, %esp

    /* Call the Nux kernel main function (defined in kernel.nux) */
    /* Wait, the Nux compiler exports `kmain` since it has @[entry] */
    call kmain

    /* If kmain returns, loop endlessly */
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
