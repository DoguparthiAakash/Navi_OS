#include <stdint.h>
#include <stdbool.h>


#include "vga.h"
#include "keyboard.h"
#include "ramfs.h"

// A very minimal string comparison for the shell
bool strcmp(uint8_t* s1, uint8_t* s2) {
    uint32_t i = 0;
    while (true) {
        uint8_t c1 = 0;
        uint8_t c2 = 0;
        __asm__("mov %1, %%eax; movb (%%eax), %%cl; mov %%cl, %0" : "=r"(c1) : "r"(s1 + i) : "%eax", "%ecx");
        __asm__("mov %1, %%eax; movb (%%eax), %%cl; mov %%cl, %0" : "=r"(c2) : "r"(s2 + i) : "%eax", "%ecx");
        
        if (c1 != c2) { return false; }
        if (c1 == 0) { return true; }
        i += 1;
    }
    return false;
}

// Starts with prefix comparison
bool starts_with(uint8_t* s1, uint8_t* prefix) {
    uint32_t i = 0;
    while (true) {
        uint8_t c1 = 0;
        uint8_t c2 = 0;
        __asm__("mov %1, %%eax; movb (%%eax), %%cl; mov %%cl, %0" : "=r"(c1) : "r"(s1 + i) : "%eax", "%ecx");
        __asm__("mov %1, %%eax; movb (%%eax), %%cl; mov %%cl, %0" : "=r"(c2) : "r"(prefix + i) : "%eax", "%ecx");
        
        if (c2 == 0) { return true; } // Reached end of prefix
        if (c1 != c2) { return false; }
        if (c1 == 0) { return false; }
        i += 1;
    }
    return false;
}

void start_shell() {
    print("Welcome to Navi OS\n");
    print("Type 'help' for commands.\n\n");
    
    // Initialize the RamFS
    init_ramfs();
    uint32_t root = get_root();
    
    uint8_t* cmd_buffer = 0x200000;
    
    while (true) {
        print("root@navi:~// ");
        
        uint32_t buf_idx = 0;
        uint32_t cmd_ready = false;
        
        while (!cmd_ready) {
            uint32_t scancode = get_scancode();
            uint32_t ascii = scancode_to_ascii(scancode);
            
            if (ascii != 0) {
                if (ascii == 10) { // Enter
                    print_char(10);
                    __asm__("movb $0, (%0)" : : "r"(cmd_buffer + buf_idx));
                    cmd_ready = true;
                } else {
                    print_char(ascii);
                    __asm__("movb %1, (%0)" : : "r"(cmd_buffer + buf_idx), "r"(ascii));
                    buf_idx += 1;
                }
            }
        }
        
        if (buf_idx == 0) {
            continue;
        }
        
        if (strcmp(cmd_buffer, "help")) {
            print("Commands: ls, cat [file], echo [msg], uname, whoami, clear, halt\n");
        } else if (strcmp(cmd_buffer, "ls")) {
            uint32_t i = 0;
            while (i < root.file_count) {
                uint8_t* fname = 0;
                // root.files[i].name
                __asm__("\n\t" "                    mov %1, %%eax\n\t" "                    mov %%eax, %%edx\n\t" "                    mov 0(%%edx), %%eax\n\t" "                    mov %%eax, %0\n\t" "                " : "=r"(fname) : "r"(root.files + (i * 12)) : "%eax", "%edx");
                print(fname);
                print("  ");
                i += 1;
            }
            print("\n");
        } else if (starts_with(cmd_buffer, "cat ")) {
            // Extract filename (skip "cat ")
            uint8_t* fname = cmd_buffer + 4;
            uint32_t i = 0;
            uint32_t found = false;
            while (i < root.file_count) {
                uint8_t* curr_name = 0;
                uint8_t* curr_data = 0;
                __asm__("\n\t" "                    mov %2, %%eax\n\t" "                    mov 0(%%eax), %%edx\n\t" "                    mov %%edx, %0\n\t" "                    mov 4(%%eax), %%edx\n\t" "                    mov %%edx, %1\n\t" "                " : "=r"(curr_name), "=r"(curr_data) : "r"(root.files + (i * 12)) : "%eax", "%edx");
                
                if (strcmp(fname, curr_name)) {
                    print(curr_data);
                    print("\n");
                    found = true;
                    break;
                }
                i += 1;
            }
            if (!found) {
                print("cat: ");
                print(fname);
                print(": No such file or directory\n");
            }
        } else if (starts_with(cmd_buffer, "echo ")) {
            print(cmd_buffer + 5);
            print("\n");
        } else if (strcmp(cmd_buffer, "uname") || strcmp(cmd_buffer, "uname -a")) {
            print("Navi OS 1.0.0 i686 (Pure Nux Kernel)\n");
        } else if (strcmp(cmd_buffer, "whoami")) {
            print("root\n");
        } else if (strcmp(cmd_buffer, "clear")) {
            clear_screen();
        } else if (strcmp(cmd_buffer, "halt")) {
            print("System halted.\n");
            __asm__("cli; hlt");
        } else {
            print("bash: ");
            print(cmd_buffer);
            print(": command not found\n");
        }
    }
}
