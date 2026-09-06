#include <stdint.h>
#include <stdbool.h>


#include "fs.h"

// ramfs.nux - Initial RAM File System
// This module reads an archive loaded into memory by GRUB.
// GRUB passes the address of boot modules via the Multiboot struct.

// Hardcoded for the demo: We assume the initrd module is loaded at 0x01000000
// and it contains a flat sequence of files: [Name (16 bytes)][Size (4 bytes)][Data (Size bytes)]

uint8_t* INITRD_BASE = 0x01000000;
uint32_t MAX_FILES = 10;
var root_files: *File = 0x01100000; // Static allocation for file nodes
uint32_t root_file_count = 0;

var root_dir: Directory;

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
    
    root_dir.init("/", root_files, root_file_count);
}

Directory get_root() {
    return root_dir;
}
