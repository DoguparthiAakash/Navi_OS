.global _syscall_isr
.extern syscall_dispatcher

_syscall_isr:
    # Save all general purpose registers
    pushal

    # Pass registers as arguments to syscall_dispatcher(eax, ebx, ecx, edx)
    # The stack grows downwards, so push in reverse order
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax

    call syscall_dispatcher

    # Clean up arguments (4 * 4 bytes)
    addl $16, %esp

    # Restore registers
    popal
    
    # Return from interrupt
    iret
