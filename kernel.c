#include <stdint.h>
#include <stdbool.h>


#include "vga.h"
#include "shell.h"
#include "fs.h"
#include "ramfs.h"

// The entry point called by boot.s

void kmain() {
    // Initialize hardware drivers
    clear_screen();
    
    // Hand off execution to the interactive shell
    start_shell();
    
    // We should never return here, but just in case:
    __asm__("cli; hlt");
}
