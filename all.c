#include <stdint.h>
#include <stdbool.h>
static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
typedef struct File File;
typedef struct Directory Directory;


// memory.nux
// Basic bump allocator for Navi OS

uint32_t heap_start = 0x1000000; // 16 MB mark
uint32_t heap_current = 0x1000000;

uint8_t* alloc(size: u32) {
    uint32_t ptr = heap_current;
    
    // Align to 4 bytes
    uint32_t remainder = size % 4;
    if (remainder != 0) {
        size = size + (4 - remainder);
    }
    
    heap_current = heap_current + size;
    return ptr as *u8;
}

void free(uint8_t* ptr) {
    // Bump allocator doesn't free.
    // In a real OS, implement a linked list or buddy allocator.
}


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
            
            *(VGA_BUFFER + index) = char_value;
            
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
        *(VGA_BUFFER + index) = char_value;
        
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
        uint8_t c = *(str + i);
        
        if (c == 0) {
            break;
        }
        print_char(c);
        i += 1;
    }
}


// Basic PS/2 Keyboard polling via x86 Port I/O (Port 0x60 and 0x64)



uint8_t get_scancode() {
    // Port 0x64 is the command/status port.
    // We loop until bit 0 (Output Buffer Status) is 1 (data is ready to read).
    while (true) {
        uint32_t status = __inb(0x64);
        if ((status & 1) != 0) {
            break;
        }
    }
    
    // Read the actual scancode from Port 0x60
    return __inb(0x60);
}

// Very basic US QWERTY Scancode to ASCII map (simplified)
// We only map a few keys for the demo shell.
uint8_t scancode_to_ascii(uint8_t scancode) {
    if (scancode == 0x1E) { return 97; } // 'a'
    if (scancode == 0x30) { return 98; } // 'b'
    if (scancode == 0x2E) { return 99; } // 'c'
    if (scancode == 0x20) { return 100;} // 'd'
    if (scancode == 0x12) { return 101;} // 'e'
    if (scancode == 0x21) { return 102;} // 'f'
    if (scancode == 0x22) { return 103;} // 'g'
    if (scancode == 0x23) { return 104;} // 'h'
    if (scancode == 0x17) { return 105;} // 'i'
    if (scancode == 0x24) { return 106;} // 'j'
    if (scancode == 0x25) { return 107;} // 'k'
    if (scancode == 0x26) { return 108;} // 'l'
    if (scancode == 0x32) { return 109;} // 'm'
    if (scancode == 0x31) { return 110;} // 'n'
    if (scancode == 0x18) { return 111;} // 'o'
    if (scancode == 0x19) { return 112;} // 'p'
    if (scancode == 0x10) { return 113;} // 'q'
    if (scancode == 0x13) { return 114;} // 'r'
    if (scancode == 0x1F) { return 115;} // 's'
    if (scancode == 0x14) { return 116;} // 't'
    if (scancode == 0x16) { return 117;} // 'u'
    if (scancode == 0x2F) { return 118;} // 'v'
    if (scancode == 0x11) { return 119;} // 'w'
    if (scancode == 0x2D) { return 120;} // 'x'
    if (scancode == 0x15) { return 121;} // 'y'
    if (scancode == 0x2C) { return 122;} // 'z'
    
    if (scancode == 0x39) { return 32; } // Space
    if (scancode == 0x1C) { return 10; } // Enter (\n)
    
    return 0; // Unknown / Key release
}


// fs.nux - Virtual File System structures

typedef struct File File;
struct File {

    uint8_t* name;
    uint8_t* data;
    uint32_t size;
};

typedef struct Directory Directory;
struct Directory {

    uint8_t* name;
    File* files; // Pointer to array of files for now
    uint32_t file_count;
};




// ramfs.nux - Initial RAM File System
// This module reads an archive loaded into memory by GRUB.
// GRUB passes the address of boot modules via the Multiboot struct.

// Hardcoded for the demo: We assume the initrd module is loaded at 0x01000000
// and it contains a flat sequence of files: [Name (16 bytes)][Size (4 bytes)][Data (Size bytes)]

uint8_t* INITRD_BASE = 0x01000000;
uint32_t MAX_FILES = 10;
File* root_files = 0x01100000; // Static allocation for file nodes
uint32_t root_file_count = 0;

Directory root_dir;

void init_ramfs() {
    // In a real OS, we read the multiboot information struct.
    // For now, we mock parsing an archive from INITRD_BASE.
    
    // We will simulate that there is one file "readme.txt" pre-loaded
    // at INITRD_BASE by our build script.
    
    // Mocking the parsed structure for the shell to use
    uint8_t* readme_name = "readme.txt";
    uint8_t* readme_data = "Welcome to Navi OS!\nThis is a real file stored in NuxFS (RamFS).\n";
    uint32_t readme_size = 65; // Length of the string above
    
    // We simulate populating the root_files array
    // root_files[0].init(readme_name, readme_data, readme_size);
    // Since we can't do array syntax easily without collections:
    __asm__("\n\t" "        mov %1, %%eax\n\t" "        mov %%eax, 0(%0)\n\t" "        mov %2, %%eax\n\t" "        mov %%eax, 4(%0)\n\t" "        mov %3, %%eax\n\t" "        mov %%eax, 8(%0)\n\t" "    " : : "r"(root_files), "r"(readme_name), "r"(readme_data), "r"(readme_size) : "%eax");
    
    root_file_count = 1;
    
root_dir.name = "/"; root_dir.files = root_files; root_dir.file_count = root_file_count;
}

Directory get_root() {
    return root_dir;
}






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
    Directory root = get_root();
    
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
                    __asm__("movb %b1, (%0)" : : "r"(cmd_buffer + buf_idx), "q"(ascii));
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
                __asm__("\n\t" "                    mov %1, %%eax\n\t" "                    mov %%eax, %%edx\n\t" "                    mov 0(%%edx), %%eax\n\t" "                    mov %%eax, %0\n\t" "                " : "=r"(fname) : "r"((uint8_t*)root.files + (i * 12)) : "%eax", "%edx");
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
                __asm__("\n\t" "                    mov %2, %%eax\n\t" "                    mov 0(%%eax), %%edx\n\t" "                    mov %%edx, %0\n\t" "                    mov 4(%%eax), %%edx\n\t" "                    mov %%edx, %1\n\t" "                " : "=r"(curr_name), "=r"(curr_data) : "r"((uint8_t*)root.files + (i * 12)) : "%eax", "%edx");
                
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







// The entry point called by boot.s

void kmain() {
    // Initialize hardware drivers
    clear_screen();
    
    // Hand off execution to the interactive shell
    start_shell();
    
    // We should never return here, but just in case:
    __asm__("cli; hlt");
}
