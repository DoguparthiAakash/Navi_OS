#include <stdint.h>
#include <stdbool.h>


// VGA Text Mode memory mapped I/O at 0xB8000

uint16_t* VGA_BUFFER = 0xB8000;
uint32_t VGA_WIDTH = 80;
uint32_t VGA_HEIGHT = 25;

uint32_t cursor_x = 0;
uint32_t cursor_y = 0;
uint8_t current_color = 0x0F; // White on Black

// Nux natively supports inline assembly and raw pointer writes.
// We will simulate memory writes assuming the Nux compiler turns `*ptr = val` into a write.

void clear_screen() {
    uint32_t y = 0;
    while (y < VGA_HEIGHT) {
        uint32_t x = 0;
        while (x < VGA_WIDTH) {
            uint32_t index = y * VGA_WIDTH + x;
            // A blank character with the current color
            uint16_t char_value = (current_color << 8) | 0x20; 
            
            // Write to raw pointer (simulating Nux pointer dereference)
            // This is equivalent to volatile writes in C.
            // *(VGA_BUFFER + index) = char_value;
            __asm__("\n\t" "                mov %0, %%eax\n\t" "                mov %1, %%edx\n\t" "                mov %%ax, (%%edx)\n\t" "            " : : "r"(char_value), "r"(VGA_BUFFER + index * 2));
            
            x += 1;
        }
        y += 1;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void print_char(uint8_t c) {
    if (c == 10) { // Newline (\n)
        cursor_x = 0;
        cursor_y += 1;
    } else {
        uint32_t index = cursor_y * VGA_WIDTH + cursor_x;
        uint16_t char_value = (current_color << 8) | c;
        
        __asm__("\n\t" "            mov %0, %%eax\n\t" "            mov %1, %%edx\n\t" "            mov %%ax, (%%edx)\n\t" "        " : : "r"(char_value), "r"(VGA_BUFFER + index * 2));
        
        cursor_x += 1;
    }
    
    // Scroll if we hit the bottom
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y += 1;
    }
    if (cursor_y >= VGA_HEIGHT) {
        // Primitive scrolling logic omitted for brevity, just reset to top
        clear_screen();
    }
}

void print(uint8_t* str) {
    uint32_t i = 0;
    // We assume string is null terminated.
    while (true) {
        // Read byte from raw pointer
        uint8_t c = 0;
        __asm__("\n\t" "            mov %1, %%eax\n\t" "            movb (%%eax), %%cl\n\t" "            mov %%cl, %0\n\t" "        " : "=r"(c) : "r"(str + i) : "%eax", "%ecx");
        
        if (c == 0) {
            break;
        }
        print_char(c);
        i += 1;
    }
}
