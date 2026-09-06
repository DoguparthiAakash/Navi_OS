#include <stdint.h>
#include <stdbool.h>


// Basic PS/2 Keyboard polling via x86 Port I/O (Port 0x60 and 0x64)

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__("\n\t" "        inb %1, %0\n\t" "    " : "=a"(result) : "Nd"(port));
    return result;
}

uint8_t get_scancode() {
    // Port 0x64 is the command/status port.
    // We loop until bit 0 (Output Buffer Status) is 1 (data is ready to read).
    while (true) {
        uint32_t status = inb(0x64);
        if ((status & 1) != 0) {
            break;
        }
    }
    
    // Read the actual scancode from Port 0x60
    return inb(0x60);
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
