#include <stdint.h>
#include <stdbool.h>
static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
typedef struct File File;
typedef struct Directory Directory;
/// hw — Nux Hardware Abstraction Layer
///
/// Provides hardware-independent access to low-level CPU and I/O operations.
/// This replaces raw `__asm__(...)` blocks throughout Nux programs.
///
/// The native compiler backend (native_backend.rs) maps these functions
/// directly to the correct x86/ARM/RISC-V instructions — the programmer
/// never writes architecture-specific code.
///
/// Usage:
///   
///   outb(0x3F8, 65);    // write 'A' to COM1
///   uint32_t c = inb(0x60);  // read keyboard scancode



// ─── I/O Port Access ──────────────────────────────────────────────────────────

/// Write a byte to a hardware I/O port.
/// Equivalent to x86 `out dx, al` instruction.
/// port: the 16-bit I/O port address
/// val:  the byte value to write

static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }

/// Read a byte from a hardware I/O port.
/// Equivalent to x86 `in al, dx` instruction.
/// port: the 16-bit I/O port address
/// Returns the byte read from the port.

static inline uint8_t inb(uint16_t port) { uint8_t ret; __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }

/// Write a 16-bit word to a hardware I/O port.

static inline void outw(uint16_t port, uint16_t val) { __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port)); }

/// Read a 16-bit word from a hardware I/O port.

static inline uint16_t inw(uint16_t port) { uint16_t ret; __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }

/// Write a 32-bit dword to a hardware I/O port.

void outd(uint16_t port, uint32_t val);

/// Read a 32-bit dword from a hardware I/O port.

uint32_t ind(uint16_t port);

// ─── I/O Wait ─────────────────────────────────────────────────────────────────

/// Insert an I/O delay (write to an unused port).
/// Used after I/O port writes on older hardware to give the device time to respond.
void io_wait() {
    outb(0x80, 0);
}

// ─── CPU Control ──────────────────────────────────────────────────────────────

/// Disable hardware interrupts (cli).
/// Use before critical sections that must not be interrupted.

static inline void cli() { __asm__ volatile ("cli"); }

/// Enable hardware interrupts (sti).

static inline void sti() { __asm__ volatile ("sti"); }

/// Halt the CPU until the next interrupt (hlt).
/// Used in idle loops: while(true) { hlt(); }

static inline void hlt() { __asm__ volatile ("hlt"); }

/// Full CPU halt — disables interrupts then halts forever.
/// Use for fatal kernel panics.
void halt_forever() {
    cli();
    while (true) {
        hlt();
    }
}

// ─── Memory-Mapped I/O ────────────────────────────────────────────────────────

/// Write a byte to a memory-mapped hardware register.
/// addr: the physical address of the register
/// val:  the byte to write

void mmio_write8(uint8_t* addr, uint8_t val);

/// Read a byte from a memory-mapped hardware register.

uint8_t mmio_read8(uint8_t* addr);

/// Write a 32-bit value to a memory-mapped hardware register.

void mmio_write32(uint32_t* addr, uint32_t val);

/// Read a 32-bit value from a memory-mapped hardware register.

uint32_t mmio_read32(uint32_t* addr);

// ─── CPU Information ──────────────────────────────────────────────────────────

/// Returns the current CPU timestamp counter (rdtsc).
/// Useful for performance measurements.

uint32_t rdtsc();

// ─── Common Hardware Addresses (x86 PC) ──────────────────────────────────────

// VGA text buffer (color text mode)
uint16_t* VGA_BUFFER = 0xB8000;
uint32_t VGA_WIDTH = 80;
uint32_t VGA_HEIGHT = 25;

// PIC (Programmable Interrupt Controller) ports
uint16_t PIC1_COMMAND = 0x20;
uint16_t PIC1_DATA = 0x21;
uint16_t PIC2_COMMAND = 0xA0;
uint16_t PIC2_DATA = 0xA1;
uint8_t PIC_EOI = 0x20;  // End-of-interrupt signal

// PS/2 Keyboard
uint16_t KEYBOARD_DATA_PORT = 0x60;
uint16_t KEYBOARD_STATUS_PORT = 0x64;

// Serial port COM1
uint16_t COM1 = 0x3F8;

// PIT (Programmable Interval Timer)
uint16_t PIT_CHANNEL0 = 0x40;
uint16_t PIT_COMMAND = 0x43;

// ─── PIC Helpers ─────────────────────────────────────────────────────────────

/// Send End-of-Interrupt signal to PIC.
/// irq: the interrupt number (0-15)
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/// Remap PIC IRQs to avoid conflict with CPU exception vectors.
/// offset1: new base for master PIC (typically 0x20)
/// offset2: new base for slave PIC  (typically 0x28)
void pic_remap(uint8_t offset1, uint8_t offset2) {
    // Save masks
    uint32_t mask1 = inb(PIC1_DATA);
    uint32_t mask2 = inb(PIC2_DATA);

    // Start initialization sequence
    outb(PIC1_COMMAND, 0x11);  io_wait();
    outb(PIC2_COMMAND, 0x11);  io_wait();

    // Set vector offsets
    outb(PIC1_DATA, offset1);  io_wait();
    outb(PIC2_DATA, offset2);  io_wait();

    // Tell PICs about each other
    outb(PIC1_DATA, 0x04);     io_wait();  // slave at IRQ2
    outb(PIC2_DATA, 0x02);     io_wait();  // cascade identity

    // 8086 mode
    outb(PIC1_DATA, 0x01);     io_wait();
    outb(PIC2_DATA, 0x01);     io_wait();

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/// mem — Nux Memory Management
///
/// Provides memory allocation and management for both bare-metal (kernel)
/// and user-space (Linux/Windows) contexts.



// ─── Bare-metal Bump Allocator ────────────────────────────────────────────────

uint8_t* HEAP_START = 0x00400000;  // 4 MB — above kernel, below 16MB
uint8_t* HEAP_END = 0x01000000;  // 16 MB
uint8_t* heap_ptr = 0x00400000;  // cursor (same as start initially)

// ─── Primitives ───────────────────────────────────────────────────────────────

/// Allocate `size` bytes from the heap.

static uint8_t* __heap_ptr = (uint8_t*)0x00400000; static inline uint8_t* alloc(uint32_t size) { uint32_t aligned = (size + 7) & ~7u; uint8_t* p = __heap_ptr; __heap_ptr += aligned; return p; }

/// Free a previously allocated block.

static inline void free(uint8_t* ptr) { (void)ptr; }

/// Copy `n` bytes from `src` to `dst`.

static inline void copy(uint8_t* dst, uint8_t* src, uint32_t n) { for(uint32_t _i=0;_i<n;_i++) dst[_i]=src[_i]; }

/// Fill `n` bytes at `dst` with the byte value `val`.

static inline void set(uint8_t* dst, uint8_t val, uint32_t n) { for(uint32_t _i=0;_i<n;_i++) dst[_i]=val; }

/// Compare `n` bytes between `a` and `b`.

static inline uint32_t cmp(uint8_t* a, uint8_t* b, uint32_t n) { for(uint32_t _i=0;_i<n;_i++) { if(a[_i]!=b[_i]) return (uint32_t)((int)a[_i]-(int)b[_i]); } return 0; }

// ─── Advanced Allocation ──────────────────────────────────────────────────────

/// Allocate `count` elements of `elem_size` bytes each, zero-filled.
uint8_t* calloc(uint32_t count, uint32_t elem_size) {
    uint32_t total = count * elem_size;
    uint8_t* ptr = alloc(total);
    if (ptr != 0) {
        set(ptr, 0, total);
    }
    return ptr;
}

/// Resize a previously allocated block.
uint8_t* realloc(uint8_t* old_ptr, uint32_t old_size, uint32_t new_size) {
    uint8_t* new_ptr = alloc(new_size);
    if (new_ptr != 0) {
        uint32_t copy_size = old_size;
        if (new_size < old_size) {
            copy_size = new_size;
        }
        copy(new_ptr, old_ptr, copy_size);
    }
    free(old_ptr);
    return new_ptr;
}

/// Copy `n` bytes from `src` to `dst`, handling overlapping regions safely.
void move(uint8_t* dst, uint8_t* src, uint32_t n) {
    if (dst == src) {
        return;
    }
    if (dst < src) {
        copy(dst, src, n);
    } else {
        // Copy backwards to handle overlap
        uint32_t i = n;
        while (i > 0) {
            i -= 1;
            uint8_t c = 0;
            __asm__("mov %1, %%eax\n\t"
                "add %2, %%eax\n\t"
                "movb (%%eax), %%cl\n\t"
                "mov %%cl, %0\n\t"
                : "=r"(c) : "r"(src), "r"(i) : "%eax", "%ecx");
            __asm__("mov %0, %%eax\n\t"
                "add %1, %%eax\n\t"
                "movb %b2, (%%eax)\n\t"
                : : "r"(dst), "r"(i), "q"(c) : "%eax");
        }
    }
}

// ─── Alignment Helpers ────────────────────────────────────────────────────────

uint32_t align_up(uint32_t addr, uint32_t align) {
    return (addr + align - 1) & -(align);
}

uint32_t align_down(uint32_t addr, uint32_t align) {
    return addr & -(align);
}

bool is_aligned(uint8_t* ptr, uint32_t align) {
    uint32_t addr = ptr;
    return (addr & (align - 1)) == 0;
}

// ─── Heap Diagnostics ────────────────────────────────────────────────────────

uint32_t heap_free_bytes() {
    uint32_t end = HEAP_END;
    uint32_t cur = heap_ptr;
    if (cur >= end) {
        return 0;
    }
    return end - cur;
}

uint32_t heap_used_bytes() {
    uint32_t start = HEAP_START;
    uint32_t cur = heap_ptr;
    return cur - start;
}

void heap_reset() {
    heap_ptr = HEAP_START;
}

/// string — Nux String Operations
///
/// Provides fundamental string manipulation for Nux programs.
/// All strings in Nux are null-terminated byte arrays (*u8),
/// identical to C strings — zero runtime overhead.
///
/// Usage:
///   
///   uint32_t n = len("hello");     // 5
///   uint32_t eq = eq("hi", "hi");  // true




// ─── Length & Bounds ──────────────────────────────────────────────────────────

/// Return the length of a null-terminated string (excludes the null byte).
uint32_t len(uint8_t* s) {
    uint32_t i = 0;
    uint8_t c = 0;
    while (true) {
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(s), "r"(i) : "%eax", "%ecx");
        if (c == 0) {
            return i;
        }
        i += 1;
    }
    return 0;
}

// ─── Comparison ───────────────────────────────────────────────────────────────

/// Compare two null-terminated strings.
/// Returns true if they are identical.
bool eq(uint8_t* a, uint8_t* b) {
    uint32_t i = 0;
    while (true) {
        uint8_t ca = 0;
        uint8_t cb = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(ca) : "r"(a), "r"(i) : "%eax", "%ecx");
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(cb) : "r"(b), "r"(i) : "%eax", "%ecx");
        if (ca != cb) { return false; }
        if (ca == 0) { return true; }
        i += 1;
    }
    return false;
}

/// Returns true if `s` starts with `prefix`.
bool starts_with(uint8_t* s, uint8_t* prefix) {
    uint32_t i = 0;
    while (true) {
        uint8_t cs = 0;
        uint8_t cp = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(cs) : "r"(s), "r"(i) : "%eax", "%ecx");
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(cp) : "r"(prefix), "r"(i) : "%eax", "%ecx");
        if (cp == 0) { return true; }   // end of prefix
        if (cs != cp) { return false; }
        if (cs == 0) { return false; }
        i += 1;
    }
    return false;
}

/// Returns true if `s` ends with `suffix`.
bool ends_with(uint8_t* s, uint8_t* suffix) {
    uint32_t slen = len(s);
    uint32_t sufflen = len(suffix);
    if (sufflen > slen) { return false; }
    uint32_t offset = slen - sufflen;
    uint32_t i = 0;
    while (i < sufflen) {
        uint8_t cs = 0;
        uint8_t csuff = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(cs) : "r"(s), "r"(offset + i) : "%eax", "%ecx");
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(csuff) : "r"(suffix), "r"(i) : "%eax", "%ecx");
        if (cs != csuff) { return false; }
        i += 1;
    }
    return true;
}

// ─── Copy & Concat ────────────────────────────────────────────────────────────

/// Copy string `src` into buffer `dst`. Buffer must be large enough.
/// Null-terminates `dst`. Returns number of bytes copied (excluding null).
uint32_t str_copy(uint8_t* dst, uint8_t* src) {
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(src), "r"(i) : "%eax", "%ecx");
        __asm__("mov %0, %%eax\n\t"
            "add %1, %%eax\n\t"
            "movb %b2, (%%eax)\n\t"
            : : "r"(dst), "r"(i), "q"(c) : "%eax");
        if (c == 0) { return i; }
        i += 1;
    }
    return i;
}

/// Concatenate `src` onto the end of `dst`.
/// `dst` must have enough space for both strings + null terminator.
/// Returns total length of the resulting string.
uint32_t concat(uint8_t* dst, uint8_t* src) {
    uint32_t dst_len = len(dst);
    // Start writing src at the null terminator of dst
    uint32_t write_pos = dst_len;
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(src), "r"(i) : "%eax", "%ecx");
        __asm__("mov %0, %%eax\n\t"
            "add %1, %%eax\n\t"
            "movb %b2, (%%eax)\n\t"
            : : "r"(dst), "r"(write_pos), "q"(c) : "%eax");
        if (c == 0) { return write_pos; }
        i += 1;
        write_pos += 1;
    }
    return write_pos;
}

// ─── Search ───────────────────────────────────────────────────────────────────

/// Find first occurrence of byte `ch` in string `s`.
/// Returns offset of the character, or 0xFFFFFFFF if not found.
uint32_t find_char(uint8_t* s, uint8_t ch) {
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(s), "r"(i) : "%eax", "%ecx");
        if (c == 0) { return 0xFFFFFFFF; }
        if (c == ch) { return i; }
        i += 1;
    }
    return 0xFFFFFFFF;
}

// ─── Numeric Conversion ───────────────────────────────────────────────────────

/// Convert an unsigned 32-bit integer to a decimal ASCII string.
/// Writes result into `buf`. `buf` must be at least 11 bytes.
/// Returns number of digits written (not counting null terminator).
uint32_t u32_to_str(uint32_t val, uint8_t* buf) {
    if (val == 0) {
        __asm__("movb $48, (%0)\n\t" : : "r"(buf));     // '0'
        __asm__("movb $0,  1(%0)\n\t" : : "r"(buf));    // null
        return 1;
    }
    // Write digits in reverse
    uint8_t* tmp = buf + 20;
    __asm__("movb $0, (%0)\n\t" : : "r"(tmp));           // null term at end
    uint32_t pos = 20;
    uint32_t v = val;
    while (v > 0) {
        uint8_t digit = (v % 10) + 48;
        pos -= 1;
        __asm__("mov %0, %%eax\n\t"
            "add %1, %%eax\n\t"
            "movb %b2, (%%eax)\n\t"
            : : "r"(buf), "r"(pos), "q"(digit) : "%eax");
        v = v / 10;
    }
    // Shift result to front of buf
    uint32_t digits_len = 20 - pos;
    uint32_t i = 0;
    while (i < digits_len) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(buf), "r"(pos + i) : "%eax", "%ecx");
        __asm__("mov %0, %%eax\n\t"
            "add %1, %%eax\n\t"
            "movb %b2, (%%eax)\n\t"
            : : "r"(buf), "r"(i), "q"(c) : "%eax");
        i += 1;
    }
    __asm__("mov %0, %%eax\n\t"
        "add %1, %%eax\n\t"
        "movb $0, (%%eax)\n\t"
        : : "r"(buf), "r"(digits_len) : "%eax");
    return digits_len;
}

/// Convert a decimal ASCII string to a u32.
/// Stops at the first non-digit character.
uint32_t str_to_u32(uint8_t* s) {
    uint32_t result = 0;
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(s), "r"(i) : "%eax", "%ecx");
        if (c < 48 || c > 57) { return result; }  // not '0'-'9'
        result = result * 10 + (c - 48);
        i += 1;
    }
    return result;
}

/// io — Nux I/O Standard Library
///
/// Provides platform-independent print/read operations for both
/// bare-metal (VGA text mode) and user-space (Linux/Windows stdout).
///
/// On bare metal (Navi OS): writes to VGA text buffer at 0xB8000.
/// On user-space: the native compiler backend maps to write(1,...).
///
/// Usage:
///   
///   print("Hello, World!\n");
///   print_u32(42);





// ─── VGA Color Attributes ─────────────────────────────────────────────────────

uint8_t COLOR_BLACK = 0;
uint8_t COLOR_BLUE = 1;
uint8_t COLOR_GREEN = 2;
uint8_t COLOR_CYAN = 3;
uint8_t COLOR_RED = 4;
uint8_t COLOR_MAGENTA = 5;
uint8_t COLOR_BROWN = 6;
uint8_t COLOR_LIGHT_GREY = 7;
uint8_t COLOR_DARK_GREY = 8;
uint8_t COLOR_LIGHT_BLUE = 9;
uint8_t COLOR_LIGHT_GREEN = 10;
uint8_t COLOR_LIGHT_CYAN = 11;
uint8_t COLOR_LIGHT_RED = 12;
uint8_t COLOR_LIGHT_MAGENTA = 13;
uint8_t COLOR_LIGHT_BROWN = 14;
uint8_t COLOR_YELLOW = 14;  // alias for LIGHT_BROWN
uint8_t COLOR_WHITE = 15;

// ─── VGA State ────────────────────────────────────────────────────────────────

uint32_t vga_col = 0;
uint32_t vga_row = 0;
uint8_t vga_color = 0x0F;  // white on black

uint8_t make_vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void set_color(uint8_t fg, uint8_t bg) {
    vga_color = make_vga_color(fg, bg);
}

// ─── VGA Core ─────────────────────────────────────────────────────────────────

void vga_put_at(uint8_t ch, uint32_t col, uint32_t row) {
    uint32_t idx = row * VGA_WIDTH + col;
    uint16_t entry = ch | (vga_color << 8);
    __asm__("mov %0, %%eax\n\t"
        "mov %1, %%ecx\n\t"
        "lea (%%eax, %%ecx, 2), %%eax\n\t"
        "movw %2, (%%eax)\n\t"
        : : "r"(VGA_BUFFER), "r"(idx), "r"(entry) : "%eax", "%ecx");
}

void scroll_up() {
    // Move all rows up by one
    uint32_t row = 1;
    while (row < VGA_HEIGHT) {
        uint32_t col = 0;
        while (col < VGA_WIDTH) {
            uint32_t src_idx = row * VGA_WIDTH + col;
            uint32_t dst_idx = (row - 1) * VGA_WIDTH + col;
            uint16_t entry = 0;
            __asm__("mov %1, %%eax\n\t"
                "lea (%%eax, %2, 2), %%eax\n\t"
                "movw (%%eax), %0\n\t"
                : "=r"(entry) : "r"(VGA_BUFFER), "r"(src_idx) : "%eax");
            __asm__("mov %0, %%eax\n\t"
                "lea (%%eax, %1, 2), %%eax\n\t"
                "movw %2, (%%eax)\n\t"
                : : "r"(VGA_BUFFER), "r"(dst_idx), "r"(entry) : "%eax");
            col += 1;
        }
        row += 1;
    }
    // Clear the last row
    uint32_t col = 0;
    uint16_t blank = 0x20 | (vga_color << 8);
    while (col < VGA_WIDTH) {
        uint32_t idx = (VGA_HEIGHT - 1) * VGA_WIDTH + col;
        __asm__("mov %0, %%eax\n\t"
            "lea (%%eax, %1, 2), %%eax\n\t"
            "movw %2, (%%eax)\n\t"
            : : "r"(VGA_BUFFER), "r"(idx), "r"(blank) : "%eax");
        col += 1;
    }
}

/// Clear the entire screen (fill with spaces using current color).
void clear() {
    vga_col = 0;
    vga_row = 0;
    uint32_t row = 0;
    while (row < VGA_HEIGHT) {
        uint32_t col = 0;
        while (col < VGA_WIDTH) {
            vga_put_at(0x20, col, row);
            col += 1;
        }
        row += 1;
    }
}

// ─── Character Output ─────────────────────────────────────────────────────────

/// Print a single character.
void print_char(uint8_t ch) {
    if (ch == 10) {   // '\n' newline
        vga_col = 0;
        vga_row += 1;
    } else if (ch == 13) {  // '\r' carriage return
        vga_col = 0;
    } else if (ch == 8) {   // '\b' backspace
        if (vga_col > 0) {
            vga_col -= 1;
            vga_put_at(0x20, vga_col, vga_row);
        }
    } else if (ch == 9) {   // '\t' tab (align to 8)
        vga_col = (vga_col + 8) & -(8);
    } else {
        vga_put_at(ch, vga_col, vga_row);
        vga_col += 1;
    }

    // Wrap at end of line
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row += 1;
    }

    // Scroll if past last row
    if (vga_row >= VGA_HEIGHT) {
        scroll_up();
        vga_row = VGA_HEIGHT - 1;
    }
}

// ─── String Output ────────────────────────────────────────────────────────────

/// Print a null-terminated string.
void print(uint8_t* str) {
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("mov %1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %%cl\n\t"
            "mov %%cl, %0\n\t"
            : "=r"(c) : "r"(str), "r"(i) : "%eax", "%ecx");
        if (c == 0) { break; }
        print_char(c);
        i += 1;
    }
}

/// Print a null-terminated string followed by a newline.
void println(uint8_t* s) {
    print(s);
    print_char(10);
}

/// Print an unsigned 32-bit integer in decimal.
void print_u32(uint32_t val) {
    uint8_t* buf = 0x00300000;  // scratch buffer in low memory
    u32_to_str(val, buf);
    print(buf);
}

/// Print an unsigned 32-bit integer in hexadecimal (with "0x" prefix).
void print_hex(uint32_t val) {
    print("0x");
    uint32_t digits = "0123456789ABCDEF";
    uint32_t shift = 28;
    uint32_t started = false;
    while (true) {
        uint32_t nibble = (val >> shift) & 0xF;
        if (nibble != 0 || started || shift == 0) {
            uint8_t digit = 0;
            __asm__("mov %1, %%eax\n\t"
                "add %2, %%eax\n\t"
                "movb (%%eax), %%cl\n\t"
                "mov %%cl, %0\n\t"
                : "=r"(digit) : "r"(digits), "r"(nibble) : "%eax", "%ecx");
            print_char(digit);
            started = true;
        }
        if (shift == 0) { break; }
        shift -= 4;
    }
}

/// Print an unsigned 32-bit integer in binary (with "0b" prefix).
void print_bin(uint32_t val) {
    print("0b");
    uint32_t i = 31;
    uint32_t started = false;
    while (true) {
        uint32_t bit = (val >> i) & 1;
        if (bit != 0 || started || i == 0) {
            if (bit == 0) {
                print_char(48);  // '0'
            } else {
                print_char(49);  // '1'
            }
            started = true;
        }
        if (i == 0) { break; }
        i -= 1;
    }
}

/// Print a panic message and halt the CPU forever.
void panic(uint8_t* msg) {
    set_color(COLOR_WHITE, COLOR_RED);
    print("\n[KERNEL PANIC] ");
    println(msg);
    halt_forever();
}

// ─── Positioned Output (for editor / file manager) ────────────────────────────

/// Write a single character directly to VGA buffer at (col, row).
void print_char_at(uint8_t ch, uint32_t col, uint32_t row) {
    vga_put_at(ch, col, row);
}

/// Print a null-terminated string starting at (col, row).
/// Does not wrap — caller must clip.
void print_at(uint8_t* s, uint32_t col, uint32_t row) {
    uint32_t i = 0;
    uint32_t c_col = col;
    while (c_col < VGA_WIDTH) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(s), "r"(i) : "%eax");
        if (c == 0) { break; }
        vga_put_at(c, c_col, row);
        c_col += 1;
        i += 1;
    }
}

/// Print a u32 at (col, row) using scratch memory at 0x00310000.
void print_u32_at(uint32_t val, uint32_t col, uint32_t row) {
    uint8_t* buf = 0x00310000;
    u32_to_str(val, buf);
    print_at(buf, col, row);
}

/// Fill an entire row with spaces (using the current VGA color attribute).
void clear_row(uint32_t row) {
    uint32_t col = 0;
    while (col < VGA_WIDTH) {
        vga_put_at(0x20, col, row);
        col += 1;
    }
}

/// Fill columns [start_col, start_col+len) on the given row with spaces.
void clear_cols(uint32_t start_col, uint32_t len, uint32_t row) {
    uint32_t col = start_col;
    uint32_t end_col = start_col + len;
    while (col < end_col && col < VGA_WIDTH) {
        vga_put_at(0x20, col, row);
        col += 1;
    }
}

/// Move the hardware text cursor to (col, row) via VGA port I/O.
void set_cursor(uint32_t col, uint32_t row) {
    uint32_t pos = row * VGA_WIDTH + col;
    // High byte of position → register 14
    uint8_t hi = 0;
    __asm__("movl %1, %%eax\n\t"
        "shrl $8, %%eax\n\t"
        "andl $0xFF, %%eax\n\t"
        "movb %%al, %b0\n\t"
        : "=q"(hi) : "r"(pos) : "%eax");
    outb(0x3D4, 14);
    outb(0x3D5, hi);
    // Low byte of position → register 15
    uint8_t lo = 0;
    __asm__("movl %1, %%eax\n\t"
        "andl $0xFF, %%eax\n\t"
        "movb %%al, %b0\n\t"
        : "=q"(lo) : "r"(pos) : "%eax");
    outb(0x3D4, 15);
    outb(0x3D5, lo);
}


/// math — Nux Math Standard Library
///
/// Basic numeric operations for Nux programs.
/// Written entirely in Nux — no external dependencies.
///
/// Usage:
///   
///   uint32_t x = abs(-5);
///   uint32_t y = pow(2, 10);    // 1024
///   uint32_t z = clamp(x, 0, 100);



// ─── Basic Arithmetic ─────────────────────────────────────────────────────────

/// Return the absolute value of a signed 32-bit integer.
uint32_t abs(uint32_t x) {
    // Treating u32 as signed: check the sign bit
    uint32_t sign = x >> 31;
    if (sign == 0) {
        return x;
    }
    return (~x) + 1;
}

/// Return the minimum of two values.
uint32_t min(uint32_t a, uint32_t b) {
    if (a < b) { return a; }
    return b;
}

/// Return the maximum of two values.
uint32_t max(uint32_t a, uint32_t b) {
    if (a > b) { return a; }
    return b;
}

/// Clamp a value between lo and hi (inclusive).
uint32_t clamp(uint32_t val, uint32_t lo, uint32_t hi) {
    if (val < lo) { return lo; }
    if (val > hi) { return hi; }
    return val;
}

// ─── Power & Roots ────────────────────────────────────────────────────────────

/// Integer exponentiation: base^exp. Uses fast repeated squaring.
uint32_t pow(uint32_t base, uint32_t exp) {
    uint32_t result = 1;
    uint32_t b = base;
    uint32_t e = exp;
    while (e > 0) {
        if ((e & 1) != 0) {
            result = result * b;
        }
        b = b * b;
        e = e >> 1;
    }
    return result;
}

/// Integer square root (floor). Newton-Raphson.
uint32_t sqrt(uint32_t n) {
    if (n == 0) { return 0; }
    if (n == 1) { return 1; }
    uint32_t x = n;
    uint32_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/// Floor of log base 2 — position of the highest set bit.
uint32_t log2(uint32_t n) {
    if (n == 0) { return 0; }
    uint32_t result = 0;
    uint32_t v = n;
    while (v > 1) {
        v = v >> 1;
        result += 1;
    }
    return result;
}

// ─── Division ─────────────────────────────────────────────────────────────────

/// Ceiling integer division.
uint32_t div_ceil(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

/// Check if n is a power of 2.
bool is_pow2(uint32_t n) {
    if (n == 0) { return false; }
    return (n & (n - 1)) == 0;
}

/// Round n up to the nearest power of 2.
uint32_t next_pow2(uint32_t n) {
    if (n == 0) { return 1; }
    uint32_t v = n - 1;
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    v = v | (v >> 8);
    v = v | (v >> 16);
    return v + 1;
}

// ─── Bit Operations ───────────────────────────────────────────────────────────

/// Count set bits (popcount / Hamming weight).
uint32_t popcount(uint32_t n) {
    uint32_t count = 0;
    uint32_t v = n;
    while (v != 0) {
        count += v & 1;
        v = v >> 1;
    }
    return count;
}

/// Count leading zeros.
uint32_t clz(uint32_t n) {
    if (n == 0) { return 32; }
    uint32_t count = 0;
    uint32_t mask = 0x80000000;
    while ((n & mask) == 0) {
        count += 1;
        mask = mask >> 1;
    }
    return count;
}

// ─── GCD / LCM ───────────────────────────────────────────────────────────────

/// Greatest Common Divisor (Euclidean algorithm).
uint32_t gcd(uint32_t a, uint32_t b) {
    uint32_t x = a;
    uint32_t y = b;
    while (y != 0) {
        uint32_t t = y;
        y = x % y;
        x = t;
    }
    return x;
}

/// Least Common Multiple.
uint32_t lcm(uint32_t a, uint32_t b) {
    return (a / gcd(a, b)) * b;
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




// ─── fb.nux — VBE Linear Framebuffer Graphics Driver ─────────────────────────
//
// To switch from VGA text mode to graphical mode, we use the VESA BIOS
// Extensions (VBE). The Multiboot2 spec allows us to request a framebuffer
// from the bootloader, but for Multiboot1 (which we use), we must call
// VBE int 0x10 via real-mode before switching to protected mode.
//
// Strategy: We modify boot.nux to call VBE mode-set BEFORE entering protected
// mode, then pass the framebuffer info to the kernel via the Multiboot info
// structure's framebuffer fields.
//
// Multiboot info struct framebuffer fields (at offset 88 from mbi pointer):
//   +88:  framebuffer_addr  (u64 physical address of linear framebuffer)
//   +96:  framebuffer_pitch (u32 bytes per row)
//   +100: framebuffer_width (u32 width in pixels)
//   +104: framebuffer_height(u32 height in pixels)
//   +108: framebuffer_bpp   (u8 bits per pixel)
//   +109: framebuffer_type  (u8 0=indexed, 1=RGB, 2=EGA text)
//
// The bootloader info pointer (EBX on entry) is passed from boot to kernel.
// We store it at a fixed location for access from any module.
//
// For now we store the FB state in a global struct at 0x01900000.
// Layout:
//   +0:  addr   (u32 physical linear framebuffer address)
//   +4:  pitch  (u32 bytes per row)
//   +8:  width  (u32 pixels wide)
//   +12: height (u32 pixels tall)
//   +16: bpp    (u8  bits per pixel)
//   +17: active (u8  1 if framebuffer is live, 0 if VGA text mode)

uint8_t* FB_INFO = 0x01900000;
uint32_t* MBI_STORE = 0x01900100;  // We stash the multiboot info pointer here

uint32_t FB_OFF_ADDR = 0;
uint32_t FB_OFF_PITCH = 4;
uint32_t FB_OFF_WIDTH = 8;
uint32_t FB_OFF_HEIGHT = 12;
uint32_t FB_OFF_BPP = 16;
uint32_t FB_OFF_ACTIVE = 17;

// ─── Store multiboot info pointer (called from kernel entry) ──────────────────
void fb_save_mbi(uint32_t mbi) {
    __asm__("movl %0, (%1)\n\t" : : "r"(mbi), "r"(MBI_STORE));
}

// ─── Read framebuffer parameters from Multiboot info struct ───────────────────
uint8_t fb_read_mbi() {
    uint32_t mbi = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(mbi) : "r"(MBI_STORE));
    // If nobody called fb_save_mbi(), MBI is 0 — nothing to read.
    if (mbi == 0) { return 0; }

    // Check Multiboot flags bit 12 (framebuffer info present)
    uint32_t flags = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(flags) : "r"(mbi));
    if ((flags & 0x1000) == 0) { return 0; }  // bit 12 not set

    // Read framebuffer address (at mbi + 88, stored as u64 — we take low 32 bits)
    uint32_t fb_addr_offset = 0;
    __asm__("movl %1, %0\n\t" "addl $88, %0\n\t" : "=r"(fb_addr_offset) : "r"(mbi));
    uint32_t addr = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(addr) : "r"(fb_addr_offset));

    uint32_t pitch_off = 0;
    __asm__("movl %1, %0\n\t" "addl $96, %0\n\t" : "=r"(pitch_off) : "r"(mbi));
    uint32_t pitch = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(pitch) : "r"(pitch_off));

    uint32_t width_off = 0;
    __asm__("movl %1, %0\n\t" "addl $100, %0\n\t" : "=r"(width_off) : "r"(mbi));
    uint32_t width = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(width) : "r"(width_off));

    uint32_t height_off = 0;
    __asm__("movl %1, %0\n\t" "addl $104, %0\n\t" : "=r"(height_off) : "r"(mbi));
    uint32_t height = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(height) : "r"(height_off));

    uint32_t bpp_off = 0;
    __asm__("movl %1, %0\n\t" "addl $108, %0\n\t" : "=r"(bpp_off) : "r"(mbi));
    uint8_t bpp = 0;
    __asm__("movzbl (%1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(bpp) : "r"(bpp_off) : "%eax");

    // Check type == 1 (RGB linear framebuffer)
    uint32_t type_off = 0;
    __asm__("movl %1, %0\n\t" "addl $109, %0\n\t" : "=r"(type_off) : "r"(mbi));
    uint8_t fb_type = 0;
    __asm__("movzbl (%1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(fb_type) : "r"(type_off) : "%eax");
    if (fb_type != 1) { return 0; }  // Not an RGB linear framebuffer

    // Store in our FB_INFO block
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(FB_INFO), "r"(FB_OFF_ADDR),   "r"(addr)   : "%eax");
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(FB_INFO), "r"(FB_OFF_PITCH),  "r"(pitch)  : "%eax");
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(FB_INFO), "r"(FB_OFF_WIDTH),  "r"(width)  : "%eax");
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(FB_INFO), "r"(FB_OFF_HEIGHT), "r"(height) : "%eax");

    uint8_t active = 1;
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movb %b2, (%%eax)\n\t"
        : : "r"(FB_INFO), "r"(FB_OFF_ACTIVE), "q"(active) : "%eax");

    return 1;
}

// ─── Get framebuffer parameters ────────────────────────────────────────────────
uint32_t fb_get_width() {
    uint32_t v = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(v) : "r"(FB_INFO), "r"(FB_OFF_WIDTH) : "%eax");
    return v;
}
uint32_t fb_get_height() {
    uint32_t v = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(v) : "r"(FB_INFO), "r"(FB_OFF_HEIGHT) : "%eax");
    return v;
}
uint32_t fb_get_pitch() {
    uint32_t v = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(v) : "r"(FB_INFO), "r"(FB_OFF_PITCH) : "%eax");
    return v;
}
uint32_t fb_get_addr() {
    uint32_t v = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(v) : "r"(FB_INFO), "r"(FB_OFF_ADDR) : "%eax");
    return v;
}
uint8_t fb_is_active() {
    uint8_t v = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movzbl (%%eax), %%ecx\n\t" "movb %%cl, %b0\n\t"
        : "=q"(v) : "r"(FB_INFO), "r"(FB_OFF_ACTIVE) : "%eax", "%ecx");
    return v;
}

uint8_t* BACKBUFFER = 0x01400000;

// ─── Put a single pixel (32bpp) ───────────────────────────────────────────────
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (fb_is_active() == 0) { return; }
    uint32_t pitch = fb_get_pitch();

    // pixel_offset = y * pitch + x * 4
    uint32_t row = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(row) : "r"(y), "r"(pitch));
    uint32_t col = 0;
    __asm__("movl %1, %0\n\t" "shll $2, %0\n\t" : "=r"(col) : "r"(x));
    uint32_t pixel_addr = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(pixel_addr) : "r"(BACKBUFFER), "r"(row), "r"(col));

    __asm__("movl %1, (%0)\n\t" : : "r"(pixel_addr), "r"(color));
}

// ─── Swap Buffers (Copy Backbuffer to VRAM) ───────────────────────────────────
void fb_swap_buffers() {
    if (fb_is_active() == 0) { return; }
    uint32_t h = fb_get_height();
    uint32_t pitch = fb_get_pitch();
    uint32_t fb_addr = fb_get_addr();
    
    uint32_t bytes = h * pitch;
    uint32_t dwords = bytes >> 2;
    
    __asm__("cld\n\t"
        "rep movsl\n\t"
        : 
        : "S"(BACKBUFFER), "D"(fb_addr), "c"(dwords)
        : "memory");
}

// ─── Fill a rectangle ──────────────────────────────────────────────────────────
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    uint32_t row = 0;
    while (row < h) {
        uint32_t col = 0;
        while (col < w) {
            fb_put_pixel(x + col, y + row, color);
            col += 1;
        }
        row += 1;
    }
}

// ─── Clear screen to a color ───────────────────────────────────────────────────
void fb_clear(uint32_t color) {
    if (fb_is_active() == 0) { return; }
    fb_fill_rect(0, 0, fb_get_width(), fb_get_height(), color);
}

// ─── Draw a horizontal line ────────────────────────────────────────────────────
void fb_draw_hline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    uint32_t i = 0;
    while (i < len) {
        fb_put_pixel(x + i, y, color);
        i += 1;
    }
}

// ─── Draw a vertical line ──────────────────────────────────────────────────────
void fb_draw_vline(uint32_t x, uint32_t y, uint32_t len, uint32_t color) {
    uint32_t i = 0;
    while (i < len) {
        fb_put_pixel(x, y + i, color);
        i += 1;
    }
}

// ─── Draw a rectangle outline ─────────────────────────────────────────────────
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    fb_draw_hline(x, y,         w, color);   // top
    fb_draw_hline(x, y + h - 1, w, color);   // bottom
    fb_draw_vline(x,         y, h, color);   // left
    fb_draw_vline(x + w - 1, y, h, color);   // right
}

// ─── Draw an ARGB image from memory ──────────────────────────────────────────
// data_ptr points to: u32 width, u32 height, then width*height u32 ARGB pixels
void fb_draw_image(uint32_t px, uint32_t py, uint32_t data_ptr) {
    if (fb_is_active() == 0) { return; }
    if (data_ptr == 0) { return; }

    uint32_t w = 0;
    uint32_t h = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(w) : "r"(data_ptr));
    
    uint32_t h_ptr = 0;
    __asm__("movl %1, %0\n\t" "addl $4, %0\n\t" : "=r"(h_ptr) : "r"(data_ptr));
    __asm__("movl (%1), %0\n\t" : "=r"(h) : "r"(h_ptr));

    uint32_t pixel_ptr = 0;
    __asm__("movl %1, %0\n\t" "addl $8, %0\n\t" : "=r"(pixel_ptr) : "r"(data_ptr));

    uint32_t row = 0;
    while (row < h) {
        uint32_t col = 0;
        while (col < w) {
            uint32_t color = 0;
            __asm__("movl (%1), %0\n\t" : "=r"(color) : "r"(pixel_ptr));
            
            // Simple alpha test (alpha > 128)
            uint32_t alpha = 0;
            __asm__("movl %1, %0\n\t" "shrl $24, %0\n\t" : "=r"(alpha) : "r"(color));
            
            if (alpha > 128) {
                fb_put_pixel(px + col, py + row, color);
            }
            
            col += 1;
            pixel_ptr += 4;
        }
        row += 1;
    }
}

// ─── VRAM Direct Drawing (for mouse cursor optimization) ──────────────────────
void fb_put_pixel_vram(uint32_t x, uint32_t y, uint32_t color) {
    if (fb_is_active() == 0) { return; }
    uint32_t pitch = fb_get_pitch();
    uint32_t vram = fb_get_addr();
    uint32_t row = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(row) : "r"(y), "r"(pitch));
    uint32_t col = 0;
    __asm__("movl %1, %0\n\t" "shll $2, %0\n\t" : "=r"(col) : "r"(x));
    uint32_t pixel_addr = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(pixel_addr) : "r"(vram), "r"(row), "r"(col));
    __asm__("movl %1, (%0)\n\t" : : "r"(pixel_addr), "r"(color));
}

void fb_draw_image_vram(uint32_t px, uint32_t py, uint32_t data_ptr) {
    if (fb_is_active() == 0) { return; }
    if (data_ptr == 0) { return; }
    uint32_t w = 0;
    uint32_t h = 0;
    __asm__("movl (%1), %0\n\t" : "=r"(w) : "r"(data_ptr));
    uint32_t h_ptr = 0;
    __asm__("movl %1, %0\n\t" "addl $4, %0\n\t" : "=r"(h_ptr) : "r"(data_ptr));
    __asm__("movl (%1), %0\n\t" : "=r"(h) : "r"(h_ptr));
    uint32_t pixel_ptr = 0;
    __asm__("movl %1, %0\n\t" "addl $8, %0\n\t" : "=r"(pixel_ptr) : "r"(data_ptr));
    uint32_t row = 0;
    while (row < h) {
        uint32_t col = 0;
        while (col < w) {
            uint32_t color = 0;
            __asm__("movl (%1), %0\n\t" : "=r"(color) : "r"(pixel_ptr));
            uint32_t alpha = 0;
            __asm__("movl %1, %0\n\t" "shrl $24, %0\n\t" : "=r"(alpha) : "r"(color));
            if (alpha > 128) {
                fb_put_pixel_vram(px + col, py + row, color);
            }
            col += 1;
            pixel_ptr += 4;
        }
        row += 1;
    }
}

// ─── Restore rect from BACKBUFFER to VRAM ────────────────────────────────────
void fb_restore_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (fb_is_active() == 0) { return; }
    uint32_t pitch = fb_get_pitch();
    uint32_t vram = fb_get_addr();
    uint32_t max_w = fb_get_width();
    uint32_t max_h = fb_get_height();

    uint32_t row = 0;
    while (row < h) {
        uint32_t cy = y + row;
        if (cy >= max_h) { break; }

        uint32_t offset_y = 0;
        __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(offset_y) : "r"(cy), "r"(pitch));
        uint32_t offset_x = 0;
        __asm__("movl %1, %0\n\t" "shll $2, %0\n\t" : "=r"(offset_x) : "r"(x));

        uint32_t src_ptr = 0;
        __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" "addl %3, %0\n\t" : "=r"(src_ptr) : "r"(BACKBUFFER), "r"(offset_y), "r"(offset_x));
        uint32_t dst_ptr = 0;
        __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" "addl %3, %0\n\t" : "=r"(dst_ptr) : "r"(vram), "r"(offset_y), "r"(offset_x));

        uint32_t col = 0;
        while (col < w) {
            uint32_t cx = x + col;
            if (cx >= max_w) { break; }
            uint32_t pixel = 0;
            __asm__("movl (%1), %0\n\t" : "=r"(pixel) : "r"(src_ptr));
            __asm__("movl %1, (%0)\n\t" : : "r"(dst_ptr), "r"(pixel));
            src_ptr += 4;
            dst_ptr += 4;
            col += 1;
        }
        row += 1;
    }
}







// ─── font.nux — PSF (PC Screen Font) Bitmap Font Renderer ─────────────────────
//
// PSF1 font format:
//   Header (4 bytes): magic[2], mode, charsize
//   Glyph data: 256 glyphs, each `charsize` bytes tall, 8 pixels wide
//
// We embed a minimal 8x16 font for the ASCII printable range (32-127).
// Each character is 8 wide x 16 tall = 16 bytes of bitmap data.
//
// Since we cannot load files from disk yet, we embed the IBM VGA 8x16 font
// data directly. Each byte in the font is one row of 8 horizontal pixels,
// bit 7 = leftmost pixel.

uint32_t FONT_WIDTH = 8;
uint32_t FONT_HEIGHT = 16;
uint8_t* FONT_DATA = 0x01A00000;  // 256 * 16 = 4096 bytes
uint8_t FONT_LOADED = 0;

// Foreground/background colors for text rendering
uint32_t TEXT_FG = 0xFFFFFF;  // White
uint32_t TEXT_BG = 0x1E1E2E;  // Dark navy (Catppuccin Mocha base)

// ─── Copy font bitmap data to FONT_DATA region ────────────────────────────────
// This copies a minimal hardcoded font for ASCII 32-127.
// A real implementation would load a PSF file from disk.
void _copy_font_byte(uint32_t idx, uint8_t val) {
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(FONT_DATA), "r"(idx), "q"(val));
}

void _load_minimal_font() {
    // Copy from asset_font linked symbol
    uint8_t* src = 0;
    __asm__("movl $asset_font, %0\n\t" : "=r"(src));
    
    if (src == 0) { return; }
    
    uint32_t i = 0;
    while (i < 4096) {
        uint8_t val = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(val) : "r"(src), "r"(i) : "%eax");
        __asm__("movb %b2, (%0,%1,1)\n\t"
            : : "r"(FONT_DATA), "r"(i), "q"(val));
        i += 1;
    }

    FONT_LOADED = 1;
}

void init_font() {
    _load_minimal_font();
}

// ─── Draw one character glyph to framebuffer ──────────────────────────────────
void font_draw_char(uint8_t ch, uint32_t px, uint32_t py, uint32_t fg, uint32_t bg) {
    if (FONT_LOADED == 0) { return; }

    uint32_t ascii = ch;
    if (ascii < 32 || ascii > 127) { ascii = 63; }  // fallback to '?'

    uint32_t glyph_base = 0;
    __asm__("movl %1, %0\n\t" "imull $16, %0\n\t" : "=r"(glyph_base) : "r"(ascii));

    uint32_t row = 0;
    while (row < FONT_HEIGHT) {
        uint32_t glyph_idx = 0;
        __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(glyph_idx) : "r"(glyph_base), "r"(row));
        uint8_t glyph_byte = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(glyph_byte) : "r"(FONT_DATA), "r"(glyph_idx) : "%eax");

        uint32_t col = 0;
        while (col < FONT_WIDTH) {
            // Check bit (7 - col) of glyph_byte
            uint32_t shift = 7 - col;
            uint8_t bit = 0;
            uint32_t zero_r = 0;
            __asm__("movzbl %b2, %%eax\n\t" "shrl %b3, %%eax\n\t" "andl $1, %%eax\n\t" "movb %%al, %b0\n\t"
                : "=q"(bit) : "r"(zero_r), "q"(glyph_byte), "c"(shift) : "%eax");
            if (bit != 0) {
                fb_put_pixel(px + col, py + row, fg);
            } else {
                fb_put_pixel(px + col, py + row, bg);
            }
            col += 1;
        }
        row += 1;
    }
}

// ─── Draw a string of text ─────────────────────────────────────────────────────
void font_draw_string(uint8_t* s, uint32_t px, uint32_t py, uint32_t fg, uint32_t bg) {
    uint32_t i = 0;
    uint32_t x = px;
    while (true) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(s), "r"(i) : "%eax");
        if (c == 0) { break; }
        if (c == 10) {  // newline
            // For a real terminal, handle this properly
            break;
        }
        font_draw_char(c, x, py, fg, bg);
        x += FONT_WIDTH;
        i += 1;
    }
}




// --- rtc.nux -- CMOS Real-Time Clock Driver -----------------------------------
//
// The BIOS stores the system clock in CMOS RAM, accessible via:
//   I/O port 0x70: index register (select which CMOS register to read)
//   I/O port 0x71: data register  (read the selected register)
//
// Registers (values are in BCD by default unless Status B bit2 is set):
//   0x00 = Seconds    0x02 = Minutes    0x04 = Hours
//   0x07 = Day        0x08 = Month      0x09 = Year (00-99)
//   0x0A = Status A   (bit7 = update-in-progress flag)
//   0x0B = Status B   (bit2 = binary mode, bit1 = 24hr mode)
//
// We always wait for update-not-in-progress before reading.

// Cached RTC values (updated each call to rtc_read_time)
uint8_t rtc_sec = 0;
uint8_t rtc_min = 0;
uint8_t rtc_hour = 0;
uint8_t rtc_day = 1;
uint8_t rtc_mon = 1;
uint8_t rtc_year = 0;   // 2-digit year from CMOS
uint8_t rtc_cent = 20;  // century (default 20 = year 20xx)
uint8_t rtc_wday = 1;   // 1=Sunday ... 7=Saturday

// Internal: read one CMOS register
uint8_t _cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

// Internal: wait for CMOS update to complete (bit7 of Status A)
void _rtc_wait_ready() {
    uint32_t tries = 10000;
    while (tries > 0) {
        uint32_t status_a = _cmos_read(0x0A);
        if ((status_a & 0x80) == 0) { return; }
        tries -= 1;
    }
}

// Internal: convert BCD to binary
uint8_t _bcd2bin(uint8_t v) {
    uint8_t hi = (v >> 4) & 0x0F;
    uint8_t lo = v & 0x0F;
    // hi * 10 + lo  (done with repeated add to avoid needing a multiply)
    uint8_t result = lo;
    uint8_t i = 0;
    while (i < hi) {
        result += 10;
        i += 1;
    }
    return result;
}

// --- Read current time/date from CMOS -----------------------------------------
// Call this before using rtc_sec, rtc_min, etc.
void rtc_read_time() {
    _rtc_wait_ready();

    uint32_t s1 = _cmos_read(0x00);
    uint32_t mn1 = _cmos_read(0x02);
    uint32_t h1 = _cmos_read(0x04);
    uint32_t d1 = _cmos_read(0x07);
    uint32_t mo1 = _cmos_read(0x08);
    uint32_t y1 = _cmos_read(0x09);
    uint32_t wd1 = _cmos_read(0x06);

    // Read Status B to check BCD vs binary mode
    uint32_t status_b = _cmos_read(0x0B);
    uint8_t is_binary = (status_b >> 2) & 1;

    if (is_binary == 0) {
        // BCD mode (most BIOS default) — convert
        rtc_sec  = _bcd2bin(s1);
        rtc_min  = _bcd2bin(mn1);
        rtc_hour = _bcd2bin(h1 & 0x7F);  // mask AM/PM bit
        rtc_day  = _bcd2bin(d1);
        rtc_mon  = _bcd2bin(mo1);
        rtc_year = _bcd2bin(y1);
        rtc_wday = wd1;
    } else {
        // Binary mode
        rtc_sec  = s1;
        rtc_min  = mn1;
        rtc_hour = h1 & 0x7F;
        rtc_day  = d1;
        rtc_mon  = mo1;
        rtc_year = y1;
        rtc_wday = wd1;
    }

    // Try century register (0x32, available on most modern hardware)
    uint32_t cent_raw = _cmos_read(0x32);
    if (cent_raw != 0) {
        if (is_binary == 0) {
            rtc_cent = _bcd2bin(cent_raw);
        } else {
            rtc_cent = cent_raw;
        }
    } else {
        rtc_cent = 20;  // default: 21st century
    }
}

// --- Digit helpers (write a u8 value as 2 ASCII digits into a buffer) ---------
// buf_ptr: pointer to a char array, offset: position to write the 2 chars
void rtc_write_digits(uint8_t val, uint8_t* buf_ptr, uint32_t offset) {
    uint8_t tens = 0;
    uint32_t v = val;
    while (v >= 10) {
        v -= 10;
        tens += 1;
    }
    uint8_t ones = v;
    uint8_t c_tens = tens + 48;  // '0' = 48
    uint8_t c_ones = ones + 48;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buf_ptr), "r"(offset),       "q"(c_tens));
    uint32_t off2 = 0;
    __asm__("movl %1, %0\n\t" "addl $1, %0\n\t" : "=r"(off2) : "r"(offset));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buf_ptr), "r"(off2), "q"(c_ones));
}

// --- Public getters -----------------------------------------------------------
uint8_t rtc_get_sec() { return rtc_sec; }
uint8_t rtc_get_min() { return rtc_min; }
uint8_t rtc_get_hour() { return rtc_hour; }
uint8_t rtc_get_day() { return rtc_day; }
uint8_t rtc_get_mon() { return rtc_mon; }
uint8_t rtc_get_year() { return rtc_year; }
uint8_t rtc_get_cent() { return rtc_cent; }
uint8_t rtc_get_wday() { return rtc_wday; }




// ─── memory.nux — Memory Management ──────────────────────────────────────────
//
// A basic physical memory manager and heap allocator for Navi OS.
// For simplicity in this early stage, we use a bump allocator for the heap,
// starting at 0x02000000 (32MB mark).

uint8_t* HEAP_BASE = 0x02000000;
uint8_t* current_heap = 0x02000000;

// Memory Management initialization
void init_memory() {
    current_heap = HEAP_BASE;
}

// Simple malloc (bump allocator)
uint8_t* mem_alloc(uint32_t size) {
    uint32_t ptr = current_heap;
    
    // Calculate new heap pointer
    uint32_t new_heap = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(new_heap) : "r"(current_heap), "r"(size));
    
    // Align to 4 bytes
    uint32_t remainder = new_heap % 4;
    if (remainder != 0) {
        new_heap = new_heap + (4 - remainder);
    }
    
    current_heap = new_heap;
    return ptr;
}

// Free (no-op in a bump allocator)
void mem_free(uint8_t* ptr) {
    // In a full memory manager, we would add the block to a free list.
    // Currently a no-op since it's a bump allocator.
}

// Get total allocated memory
uint32_t get_allocated_memory() {
    uint32_t diff = 0;
    __asm__("movl %1, %0\n\t" "subl %2, %0\n\t" : "=r"(diff) : "r"(current_heap), "r"(HEAP_BASE));
    return diff;
}




// ─── disk.nux — Disk Management (ATA PIO) ────────────────────────────────────
//
// Very basic ATA PIO driver to read/write sectors from the primary IDE bus.
// Real disk management would use interrupts, DMA, and a file system driver.

uint16_t ATA_PRIMARY_DATA = 0x1F0;
uint16_t ATA_PRIMARY_ERR = 0x1F1;
uint16_t ATA_PRIMARY_SEC = 0x1F2;
uint16_t ATA_PRIMARY_LBA_LO = 0x1F3;
uint16_t ATA_PRIMARY_LBA_MID = 0x1F4;
uint16_t ATA_PRIMARY_LBA_HI = 0x1F5;
uint16_t ATA_PRIMARY_DRV = 0x1F6;
uint16_t ATA_PRIMARY_CMD = 0x1F7;
uint16_t ATA_PRIMARY_STAT = 0x1F7;

// Wait until the drive is ready
void ata_wait_ready() {
    while (true) {
        uint32_t status = inb(ATA_PRIMARY_STAT);
        // Wait until BSY (bit 7) is clear and RDY (bit 6) is set
        if ((status & 0x80) == 0 && (status & 0x40) != 0) {
            break;
        }
    }
}

// Read a 512-byte sector using ATA PIO mode
void read_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PRIMARY_DRV, (0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_PRIMARY_SEC, 1);
    outb(ATA_PRIMARY_LBA_LO, (lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, ((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, ((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_CMD, 0x20); // Read with retry

    ata_wait_ready();

    uint32_t i = 0;
    while (i < 256) {
        uint32_t word = inw(ATA_PRIMARY_DATA);
        
        // Store low byte
        uint8_t lo = (word & 0xFF);
        uint32_t offset1 = i * 2;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buffer), "r"(offset1), "q"(lo));
        
        // Store high byte
        uint8_t hi = ((word >> 8) & 0xFF);
        uint32_t offset2 = i * 2 + 1;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buffer), "r"(offset2), "q"(hi));
        
        i += 1;
    }
}

// Write a 512-byte sector using ATA PIO mode
void write_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PRIMARY_DRV, (0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_PRIMARY_SEC, 1);
    outb(ATA_PRIMARY_LBA_LO, (lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, ((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, ((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_CMD, 0x30); // Write with retry

    ata_wait_ready();

    uint32_t i = 0;
    while (i < 256) {
        uint32_t offset1 = i * 2;
        uint8_t lo = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(lo) : "r"(buffer), "r"(offset1) : "%eax");
        
        uint32_t offset2 = i * 2 + 1;
        uint8_t hi = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(hi) : "r"(buffer), "r"(offset2) : "%eax");
        
        uint16_t word = (lo) | ((hi) << 8);
        outw(ATA_PRIMARY_DATA, word);
        
        i += 1;
    }
}





// ─── pit.nux — Programmable Interval Timer (PIT 8253/8254) ────────────────────
//
// The PIT is the key to preemptive scheduling. We program it to fire IRQ0
// at a configurable frequency. Each IRQ0 triggers the scheduler to check
// if it should switch tasks (round-robin preemption).
//
// PIT I/O Ports:
//   0x40 - Channel 0 data port (IRQ0)
//   0x41 - Channel 1 data port (DRAM refresh - legacy, unused)
//   0x42 - Channel 2 data port (PC speaker)
//   0x43 - Mode/Command register
//
// The PIT base frequency is 1,193,182 Hz.
// Divisor = 1193182 / desired_hz
// e.g., 100 Hz timer  → divisor = 11932
//       1000 Hz timer → divisor = 1193

uint16_t PIT_CH0 = 0x40;
uint16_t PIT_CMD = 0x43;

// 100 Hz tick rate (10ms per tick)
uint16_t PIT_DIVISOR = 11932;

// Total ticks since boot
uint32_t tick_count = 0;

// ─── PIC (8259A) Ports ────────────────────────────────────────────────────────
uint16_t PIT_PIC1_CMD = 0x20;
uint16_t PIT_PIC1_DATA = 0x21;
uint16_t PIT_PIC2_CMD = 0xA0;
uint16_t PIT_PIC2_DATA = 0xA1;

uint8_t PIT_EOI = 0x20;  // End-Of-Interrupt signal

// ─── IDT (Interrupt Descriptor Table) ─────────────────────────────────────────
// Each IDT entry is 8 bytes:
//  Bytes 0-1:  offset bits 0-15
//  Bytes 2-3:  segment selector (0x08 = kernel code segment in GDT)
//  Byte  4:    reserved (zero)
//  Byte  5:    type/attribute flags (0x8E = 32-bit interrupt gate, DPL=0, Present)
//  Bytes 6-7:  offset bits 16-31
//
// We allocate IDT at a fixed physical address 0x01400000 (20 MB mark)
// IDT pointer (IDTR) is at  0x01400800

uint8_t* IDT_BASE = 0x01400000;    // 256 entries * 8 bytes = 2048 bytes
uint8_t* IDTR_BASE = 0x01400800;    // 6-byte IDT descriptor

// ─── Initialize PIC ───────────────────────────────────────────────────────────
void init_pic() {
    // Start initialization sequence in cascade mode
    outb(PIT_PIC1_CMD,  0x11);  // ICW1: init + ICW4 needed
    outb(PIT_PIC2_CMD,  0x11);

    // ICW2: Vector offsets. Map IRQ0-7 to INT 0x20-0x27, IRQ8-15 to INT 0x28-0x2F
    outb(PIT_PIC1_DATA, 0x20);
    outb(PIT_PIC2_DATA, 0x28);

    // ICW3: Tell master PIC that slave is at IRQ2; tell slave its cascade identity
    outb(PIT_PIC1_DATA, 0x04);
    outb(PIT_PIC2_DATA, 0x02);

    // ICW4: 8086 mode
    outb(PIT_PIC1_DATA, 0x01);
    outb(PIT_PIC2_DATA, 0x01);

    // MASK ALL IRQs on both PICs — nothing fires until we explicitly unmask.
    // This prevents triple fault from unhandled IRQ0 after sti.
    outb(PIT_PIC1_DATA, 0xFF);  // mask all master IRQs
    outb(PIT_PIC2_DATA, 0xFF);  // mask all slave IRQs
}

// ─── Initialize PIT ───────────────────────────────────────────────────────────
void init_pit() {
    // Command: Channel 0, lo/hi byte access, Mode 3 (square wave), binary
    outb(PIT_CMD, 0x36);

    // Send divisor lo byte then hi byte
    uint8_t lo = (PIT_DIVISOR & 0xFF);
    uint8_t hi = ((PIT_DIVISOR >> 8) & 0xFF);
    outb(PIT_CH0, lo);
    outb(PIT_CH0, hi);
}

// ─── Write one IDT entry ───────────────────────────────────────────────────────
void idt_set_gate(uint32_t vector, uint32_t handler_addr) {
    uint32_t offset = vector * 8;

    uint32_t lo_addr = handler_addr & 0xFFFF;
    uint32_t hi_addr = (handler_addr >> 16) & 0xFFFF;
    uint32_t selector = 0x08;   // kernel code segment
    uint32_t flags = 0x8E;   // interrupt gate, present, DPL=0

    // Byte 0-1: low 16 bits of handler address
    uint8_t b0 = (lo_addr & 0xFF);
    uint8_t b1 = ((lo_addr >> 8) & 0xFF);
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(offset),     "q"(b0));
    uint32_t off1 = offset + 1;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off1), "q"(b1));

    // Byte 2-3: segment selector
    uint8_t b2 = (selector & 0xFF);
    uint8_t b3 = 0;
    uint32_t off2 = offset + 2;
    uint32_t off3 = offset + 3;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off2), "q"(b2));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off3), "q"(b3));

    // Byte 4: reserved = 0
    uint8_t b4 = 0;
    uint32_t off4 = offset + 4;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off4), "q"(b4));

    // Byte 5: type/attr flags
    uint8_t b5 = flags;
    uint32_t off5 = offset + 5;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off5), "q"(b5));

    // Byte 6-7: high 16 bits of handler address
    uint8_t b6 = (hi_addr & 0xFF);
    uint8_t b7 = ((hi_addr >> 8) & 0xFF);
    uint32_t off6 = offset + 6;
    uint32_t off7 = offset + 7;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off6), "q"(b6));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(off7), "q"(b7));
}

// ─── Load the IDT ─────────────────────────────────────────────────────────────
void load_idt() {
    // Write the 6-byte IDT descriptor: limit (2 bytes) + base address (4 bytes)
    // Limit = (256 * 8) - 1 = 2047 = 0x07FF
    uint32_t limit = 2047;
    uint32_t base = IDT_BASE;

    uint8_t lim_lo = (limit & 0xFF);
    uint8_t lim_hi = ((limit >> 8) & 0xFF);
    uint32_t off0 = 0;
    uint32_t off1 = 1;
    uint32_t off2 = 2;
    uint32_t off3 = 3;
    uint32_t off4 = 4;
    uint32_t off5 = 5;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off0), "q"(lim_lo));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off1), "q"(lim_hi));

    uint8_t b0 = (base & 0xFF);
    uint8_t b1 = ((base >> 8) & 0xFF);
    uint8_t b2 = ((base >> 16) & 0xFF);
    uint8_t b3 = ((base >> 24) & 0xFF);
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off2), "q"(b0));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off3), "q"(b1));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off4), "q"(b2));
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(IDTR_BASE), "r"(off5), "q"(b3));

    // Execute LIDT instruction
    __asm__("lidt (%0)\n\t" : : "r"(IDTR_BASE));
}

// ─── IRQ0 Timer Handler ────────────────────────────────────────────────────────
// Called on every PIT tick. Increments tick_count and sends EOI.
// In a full preemptive kernel this also saves/restores register context.
void irq0_timer_tick() {
    tick_count += 1;
    // Send End-Of-Interrupt to PIC1
    outb(PIT_PIC1_CMD, PIT_EOI);
}

// ─── Get current tick count ────────────────────────────────────────────────────
uint32_t get_ticks() {
    return tick_count;
}

// ─── Sleep for N milliseconds (simple busy-wait loop) ────────────────────────
// Uses a calibrated spin loop. Does NOT rely on interrupts or HLT.
// Rough calibration: ~10 million iterations ≈ 1 second on slow emulated CPU.
void sleep_ms(uint32_t ms) {
    uint32_t iters = ms * 10000;
    uint32_t i = 0;
    while (i < iters) { i += 1; }
}

// ─── Initialize PIC + PIT safely (no sti, no IDT load yet) ───────────────────
// This reprograms the PIC vector offsets and configures PIT frequency.
// We do NOT call sti here because our IDT handlers are not yet wired.
// The kernel runs in cli (interrupts disabled) mode for safety.
void init_interrupts() {
    // Zero out the IDT region
    uint32_t i = 0;
    while (i < 2048) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(IDT_BASE), "r"(i));
        i += 1;
    }

    // Reprogram PIC and set PIT frequency (all IRQs masked inside init_pic)
    init_pic();
    init_pit();

    // Load IDT descriptor so LIDT points somewhere valid.
    // All 256 entries are zero = "not present", so even if sti were called
    // and an interrupt fired, the CPU would see a not-present gate and
    // generate a GP fault instead of jumping to address 0.
    load_idt();

    // DO NOT call sti here. Interrupts remain disabled (cli from boot).
    // Call enable_timer() only once real IRQ handlers are registered.
    print("[pit] PIC remapped, PIT configured at 100 Hz. Interrupts masked.\n");
}

// ─── Enable timer interrupt (call only when IRQ0 handler is registered) ───────
void enable_timer() {
    // Unmask only IRQ0 (bit 0 = 0) on master PIC, keep everything else masked
    outb(PIT_PIC1_DATA, 0xFE);  // 11111110 = IRQ0 unmasked
    // Enable CPU interrupts
    __asm__("sti\n\t");
}




// ─── thread.nux — Thread Management ──────────────────────────────────────────
//
// Each thread has its own saved CPU context (registers) so the scheduler
// can pause one thread and resume another (preemptive multitasking).
//
// Thread context layout (saved at THREAD_STACK_BASE + tid * THREAD_CONTEXT_SIZE):
//   +0:  eax   +4:  ebx   +8:  ecx   +12: edx
//   +16: esi   +20: edi   +24: ebp   +28: esp (stack pointer)
//   +32: eip   (instruction pointer = where to resume)
//   +36: eflags
//   +40: state (0=free, 1=ready, 2=running, 3=blocked)
//   +44: owner_pid
//   +48: stack_top (the top of this thread's private stack)
//
// Thread stacks are at 0x01600000, each 16 KB apart:
//   Thread 0: 0x01600000 - 0x01603FFF
//   Thread 1: 0x01604000 - 0x01607FFF
//   ...

uint32_t MAX_THREADS = 64;
uint8_t* THREAD_CTX_BASE = 0x01500000;   // 64 * 52 = 3328 bytes for contexts
uint32_t THREAD_STACK_BASE = 0x01600000;   // Thread stacks start here
uint32_t THREAD_STACK_SIZE = 0x4000;       // 16 KB per thread
uint32_t THREAD_CTX_SIZE = 52;           // bytes per context

// Context field offsets
uint32_t CTX_EAX = 0;
uint32_t CTX_EBX = 4;
uint32_t CTX_ECX = 8;
uint32_t CTX_EDX = 12;
uint32_t CTX_ESI = 16;
uint32_t CTX_EDI = 20;
uint32_t CTX_EBP = 24;
uint32_t CTX_ESP = 28;
uint32_t CTX_EIP = 32;
uint32_t CTX_EFLAGS = 36;
uint32_t CTX_STATE = 40;
uint32_t CTX_OWNER = 44;
uint32_t CTX_STKTOP = 48;

// Thread states
uint8_t TSTATE_FREE = 0;
uint8_t TSTATE_READY = 1;
uint8_t TSTATE_RUNNING = 2;
uint8_t TSTATE_BLOCKED = 3;

// Currently running thread index
uint32_t current_thread = 0;
uint32_t next_tid = 1;

// ─── Helpers to read/write context fields ─────────────────────────────────────
void _thread_write_u32(uint32_t tid, uint32_t field_offset, uint32_t value) {
    uint32_t ctx_offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(ctx_offset) : "r"(tid), "r"(THREAD_CTX_SIZE), "r"(field_offset));
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(THREAD_CTX_BASE), "r"(ctx_offset), "r"(value) : "%eax");
}

uint32_t _thread_read_u32(uint32_t tid, uint32_t field_offset) {
    uint32_t ctx_offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(ctx_offset) : "r"(tid), "r"(THREAD_CTX_SIZE), "r"(field_offset));
    uint32_t val = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(val) : "r"(THREAD_CTX_BASE), "r"(ctx_offset) : "%eax");
    return val;
}

void _thread_write_u8(uint32_t tid, uint32_t field_offset, uint8_t value) {
    uint32_t ctx_offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(ctx_offset) : "r"(tid), "r"(THREAD_CTX_SIZE), "r"(field_offset));
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movb %b2, (%%eax)\n\t"
        : : "r"(THREAD_CTX_BASE), "r"(ctx_offset), "q"(value) : "%eax");
}

uint8_t _thread_read_u8(uint32_t tid, uint32_t field_offset) {
    uint32_t ctx_offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(ctx_offset) : "r"(tid), "r"(THREAD_CTX_SIZE), "r"(field_offset));
    uint8_t val = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movzbl (%%eax), %%ecx\n\t" "movb %%cl, %b0\n\t"
        : "=q"(val) : "r"(THREAD_CTX_BASE), "r"(ctx_offset) : "%eax", "%ecx");
    return val;
}

// ─── Initialize the thread table ──────────────────────────────────────────────
void init_threads() {
    uint32_t i = 0;
    while (i < MAX_THREADS) {
        _thread_write_u8(i, CTX_STATE, TSTATE_FREE);
        _thread_write_u32(i, CTX_OWNER, 0);
        i += 1;
    }
    // Mark thread 0 as the kernel/shell thread (already running)
    _thread_write_u8(0, CTX_STATE, TSTATE_RUNNING);
    _thread_write_u32(0, CTX_OWNER, 1);  // PID 1 (shell)
}

// ─── Create a new thread for a process ────────────────────────────────────────
// entry_point: the function address to start executing
// owner_pid:   the process that owns this thread
// Returns the tid, or MAX_THREADS on failure.
uint32_t create_thread(uint32_t entry_point, uint32_t owner_pid) {
    uint32_t i = 1;  // Skip tid 0 (kernel thread)
    while (i < MAX_THREADS) {
        uint32_t state = _thread_read_u8(i, CTX_STATE);
        if (state == TSTATE_FREE) {
            // Set up the thread's private stack
            uint32_t stack_top = 0;
            __asm__("movl %1, %0\n\t" "movl %2, %%eax\n\t" "imull %3, %%eax\n\t" "addl %%eax, %0\n\t"
                : "=r"(stack_top) : "r"(THREAD_STACK_BASE), "r"(i), "r"(THREAD_STACK_SIZE) : "%eax");

            _thread_write_u32(i, CTX_EIP,    entry_point);
            _thread_write_u32(i, CTX_ESP,    stack_top);
            _thread_write_u32(i, CTX_EBP,    stack_top);
            _thread_write_u32(i, CTX_EFLAGS, 0x200);       // IF=1 (interrupts enabled)
            _thread_write_u32(i, CTX_OWNER,  owner_pid);
            _thread_write_u32(i, CTX_STKTOP, stack_top);
            _thread_write_u8(i,  CTX_STATE,  TSTATE_READY);

            return i;
        }
        i += 1;
    }
    return MAX_THREADS; // No free slot
}

// ─── Kill a thread ────────────────────────────────────────────────────────────
uint8_t kill_thread(uint32_t tid) {
    if (tid == 0) { return 0; } // Never kill kernel thread
    uint32_t state = _thread_read_u8(tid, CTX_STATE);
    if (state == TSTATE_FREE) { return 0; }
    _thread_write_u8(tid, CTX_STATE, TSTATE_FREE);
    return 1;
}

// ─── Block/Unblock a thread ───────────────────────────────────────────────────
void block_thread(uint32_t tid) {
    _thread_write_u8(tid, CTX_STATE, TSTATE_BLOCKED);
}

void unblock_thread(uint32_t tid) {
    uint32_t state = _thread_read_u8(tid, CTX_STATE);
    if (state == TSTATE_BLOCKED) {
        _thread_write_u8(tid, CTX_STATE, TSTATE_READY);
    }
}

// ─── Print thread list ────────────────────────────────────────────────────────
void print_threads() {
    print("TID  PID  STATE    STACK_TOP\n");
    uint32_t i = 0;
    while (i < MAX_THREADS) {
        uint32_t state = _thread_read_u8(i, CTX_STATE);
        if (state != TSTATE_FREE) {
            uint32_t owner = _thread_read_u32(i, CTX_OWNER);
            uint32_t stktop = _thread_read_u32(i, CTX_STKTOP);
            print_u32(i);
            print("    ");
            print_u32(owner);
            print("    ");
            if (state == TSTATE_READY)   { print("READY   "); }
            if (state == TSTATE_RUNNING) { print("RUNNING "); }
            if (state == TSTATE_BLOCKED) { print("BLOCKED "); }
            print("0x");
            print_u32(stktop);
            print_char(10);
        }
        i += 1;
    }
}

// ─── Preemptive Round-Robin Scheduler ─────────────────────────────────────────
// Called from the PIT IRQ0 handler tick.
// Saves current context, finds next READY thread, restores its context.
// NOTE: Full context save/restore requires inline asm at the interrupt handler.
//       This is the high-level scheduler logic that selects the next thread.
uint32_t schedule_next() {
    uint32_t start = current_thread;
    uint32_t i = current_thread + 1;
    if (i >= MAX_THREADS) { i = 0; }

    while (i != start) {
        uint32_t state = _thread_read_u8(i, CTX_STATE);
        if (state == TSTATE_READY || state == TSTATE_RUNNING) {
            // Mark old thread as READY (if it was RUNNING)
            uint32_t old_state = _thread_read_u8(current_thread, CTX_STATE);
            if (old_state == TSTATE_RUNNING) {
                _thread_write_u8(current_thread, CTX_STATE, TSTATE_READY);
            }
            // Switch to new thread
            _thread_write_u8(i, CTX_STATE, TSTATE_RUNNING);
            current_thread = i;
            return i;
        }
        i += 1;
        if (i >= MAX_THREADS) { i = 0; }
    }
    // No other thread found — stay on current
    return current_thread;
}





// ─── vfs.nux — Virtual File System (VFS) Layer ────────────────────────────────
//
// The VFS is an abstraction layer that allows different filesystems (ramfs,
// ext2, fat32, etc.) to be mounted at different points in a unified directory
// tree. All user-facing file operations go through the VFS.
//
// Architecture:
//   vfs_mount[]        — mount table (which fs is at which path)
//   vfs_node[]         — inode table (represents files/dirs across all fs)
//
// VFS Node layout (stored at VFS_NODES_BASE, 64 bytes per node):
//   +0:  name[32]      — filename (null-terminated, max 31 chars)
//   +32: type          — 0=file, 1=directory, 2=symlink
//   +36: size          — file size in bytes
//   +40: fs_id         — which filesystem owns this node (0=ramfs, 1=ext2...)
//   +44: fs_inode      — inode number in the owning filesystem
//   +48: parent_idx    — index of parent directory node
//   +52: flags         — permission/attribute flags
//   +56: reserved[8]   — (padding to 64 bytes)
//
// Mount table (stored at VFS_MOUNTS_BASE, 48 bytes per entry):
//   +0:  path[32]      — mount point path (e.g. "/", "/tmp", "/dev")
//   +32: fs_id         — filesystem type ID
//   +36: flags         — mount flags (read-only, etc.)
//   +40: reserved[8]

uint32_t VFS_MAX_NODES = 256;
uint32_t VFS_MAX_MOUNTS = 8;
uint32_t VFS_NODE_SIZE = 64;
uint32_t VFS_MOUNT_SIZE = 48;

// Memory layout (above pit/thread allocations)
uint8_t* VFS_NODES_BASE = 0x01800000;   // 256 * 64 = 16384 bytes
uint8_t* VFS_MOUNTS_BASE = 0x01804100;   // 8 * 48   = 384 bytes

// Node field offsets
uint32_t VN_NAME = 0;
uint32_t VN_TYPE = 32;
uint32_t VN_SIZE = 36;
uint32_t VN_FSID = 40;
uint32_t VN_INODE = 44;
uint32_t VN_PARENT = 48;
uint32_t VN_FLAGS = 52;

// Node types
uint32_t VN_FILE = 0;
uint32_t VN_DIR = 1;
uint32_t VN_LINK = 2;

// Filesystem IDs
uint32_t FSID_NONE = 0xFFFFFFFF;
uint32_t FSID_RAMFS = 0;
uint32_t FSID_EXT2 = 1;

uint32_t vfs_node_count = 0;
uint32_t vfs_mount_count = 0;

// ─── Low-level node field accessors ───────────────────────────────────────────
void _vn_write_u32(uint32_t node_idx, uint32_t field, uint32_t val) {
    uint32_t offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(offset) : "r"(node_idx), "r"(VFS_NODE_SIZE), "r"(field));
    __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t"
        : : "r"(VFS_NODES_BASE), "r"(offset), "r"(val) : "%eax");
}

uint32_t _vn_read_u32(uint32_t node_idx, uint32_t field) {
    uint32_t offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(offset) : "r"(node_idx), "r"(VFS_NODE_SIZE), "r"(field));
    uint32_t val = 0;
    __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t"
        : "=r"(val) : "r"(VFS_NODES_BASE), "r"(offset) : "%eax");
    return val;
}

uint8_t* _vn_name_ptr(uint32_t node_idx) {
    uint32_t offset = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t"
        : "=r"(offset) : "r"(node_idx), "r"(VFS_NODE_SIZE));
    uint8_t* ptr = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
        : "=r"(ptr) : "r"(VFS_NODES_BASE), "r"(offset));
    return ptr;
}

void _vn_copy_name(uint32_t node_idx, uint8_t* name) {
    uint32_t dst = _vn_name_ptr(node_idx);
    uint32_t i = 0;
    while (i < 31) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(name), "r"(i) : "%eax");
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(dst), "r"(i), "q"(c));
        if (c == 0) { break; }
        i += 1;
    }
    // Always null-terminate at byte 31
    uint8_t null_byte = 0;
    uint32_t max_idx = 31;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(dst), "r"(max_idx), "q"(null_byte));
}

// ─── Initialize VFS ───────────────────────────────────────────────────────────
void init_vfs() {
    // Zero out node and mount tables
    uint32_t i = 0;
    while (i < VFS_MAX_NODES * VFS_NODE_SIZE) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(VFS_NODES_BASE), "r"(i));
        i += 1;
    }
    i = 0;
    while (i < VFS_MAX_MOUNTS * VFS_MOUNT_SIZE) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(VFS_MOUNTS_BASE), "r"(i));
        i += 1;
    }

    // Create root directory node "/"
    _vn_copy_name(0, "/");
    _vn_write_u32(0, VN_TYPE,   VN_DIR);
    _vn_write_u32(0, VN_SIZE,   0);
    _vn_write_u32(0, VN_FSID,   FSID_RAMFS);
    _vn_write_u32(0, VN_INODE,  0);
    _vn_write_u32(0, VN_PARENT, 0);  // root's parent is itself
    _vn_write_u32(0, VN_FLAGS,  0x755);

    vfs_node_count = 1;

    print("[vfs] Initialized. Root node mounted at /\n");
}

// ─── Create a file node in the VFS ────────────────────────────────────────────
uint32_t vfs_create_file(uint8_t* name, uint32_t parent_idx, uint32_t fs_id, uint32_t fs_inode, uint32_t size) {
    if (vfs_node_count >= VFS_MAX_NODES) { return 0xFFFFFFFF; }
    uint32_t idx = vfs_node_count;
    _vn_copy_name(idx, name);
    _vn_write_u32(idx, VN_TYPE,   VN_FILE);
    _vn_write_u32(idx, VN_SIZE,   size);
    _vn_write_u32(idx, VN_FSID,   fs_id);
    _vn_write_u32(idx, VN_INODE,  fs_inode);
    _vn_write_u32(idx, VN_PARENT, parent_idx);
    _vn_write_u32(idx, VN_FLAGS,  0x644);
    vfs_node_count += 1;
    return idx;
}

// ─── Create a directory node in the VFS ───────────────────────────────────────
uint32_t vfs_create_dir(uint8_t* name, uint32_t parent_idx, uint32_t fs_id) {
    if (vfs_node_count >= VFS_MAX_NODES) { return 0xFFFFFFFF; }
    uint32_t idx = vfs_node_count;
    _vn_copy_name(idx, name);
    _vn_write_u32(idx, VN_TYPE,   VN_DIR);
    _vn_write_u32(idx, VN_SIZE,   0);
    _vn_write_u32(idx, VN_FSID,   fs_id);
    _vn_write_u32(idx, VN_INODE,  idx);
    _vn_write_u32(idx, VN_PARENT, parent_idx);
    _vn_write_u32(idx, VN_FLAGS,  0x755);
    vfs_node_count += 1;
    return idx;
}

// ─── Find a node by name in a parent directory ────────────────────────────────
uint32_t vfs_find(uint32_t parent_idx, uint8_t* name) {
    uint32_t i = 0;
    while (i < vfs_node_count) {
        uint32_t parent = _vn_read_u32(i, VN_PARENT);
        if (parent == parent_idx && i != 0) {
            uint32_t node_name = _vn_name_ptr(i);
            if (eq(node_name, name)) {
                return i;
            }
        }
        i += 1;
    }
    return 0xFFFFFFFF; // Not found
}

// ─── List directory contents ───────────────────────────────────────────────────
void vfs_ls(uint32_t parent_idx) {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("VFS Contents of node ");
    print_u32(parent_idx);
    print(":\n");
    set_color(COLOR_WHITE, COLOR_BLACK);

    uint32_t found = 0;
    uint32_t i = 0;
    while (i < vfs_node_count) {
        uint32_t parent = _vn_read_u32(i, VN_PARENT);
        uint32_t type_ = _vn_read_u32(i, VN_TYPE);
        if (parent == parent_idx && i != parent_idx) {
            uint32_t node_name = _vn_name_ptr(i);
            if (type_ == VN_DIR) {
                set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                print("  [DIR]  ");
            } else {
                set_color(COLOR_WHITE, COLOR_BLACK);
                print("  [FILE] ");
            }
            print(node_name);
            if (type_ == VN_FILE) {
                print("  (");
                print_u32(_vn_read_u32(i, VN_SIZE));
                print(" bytes)");
            }
            print_char(10);
            set_color(COLOR_WHITE, COLOR_BLACK);
            found += 1;
        }
        i += 1;
    }
    if (found == 0) {
        print("  (empty)\n");
    }
}

// ─── Print full VFS tree ───────────────────────────────────────────────────────
void vfs_tree() {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("/\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    uint32_t i = 1;
    while (i < vfs_node_count) {
        uint32_t type_ = _vn_read_u32(i, VN_TYPE);
        uint32_t name = _vn_name_ptr(i);
        print("  ");
        if (type_ == VN_DIR) {
            set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            print(name);
            print("/");
        } else {
            print(name);
        }
        set_color(COLOR_WHITE, COLOR_BLACK);
        print_char(10);
        i += 1;
    }
}

// ─── Populate standard Unix directory tree ────────────────────────────────────
void vfs_populate_tree() {
    // Standard Unix hierarchy
    vfs_create_dir("bin",  0, FSID_RAMFS);   // Binaries
    vfs_create_dir("dev",  0, FSID_RAMFS);   // Devices
    vfs_create_dir("etc",  0, FSID_RAMFS);   // Config
    vfs_create_dir("home", 0, FSID_RAMFS);   // User homes
    vfs_create_dir("proc", 0, FSID_RAMFS);   // Process info
    vfs_create_dir("tmp",  0, FSID_RAMFS);   // Temp files
    vfs_create_dir("usr",  0, FSID_RAMFS);   // User programs
    vfs_create_dir("var",  0, FSID_RAMFS);   // Variable data

    // /etc/navi.conf  (example config file)
    uint32_t etc_idx = vfs_find(0, "etc");
    if (etc_idx != 0xFFFFFFFF) {
        vfs_create_file("navi.conf", etc_idx, FSID_RAMFS, 1, 64);
    }

    // /home/root
    uint32_t home_idx = vfs_find(0, "home");
    if (home_idx != 0xFFFFFFFF) {
        vfs_create_dir("root", home_idx, FSID_RAMFS);
    }
}






// ─── process.nux — Process, Job, and Thread Management ────────────────────────
//
// A basic process table for cooperative multitasking/job tracking.
// Layout in low memory (above ramfs):
//   0x01300000 - proc_names (16 bytes * 32 = 512 bytes)
//   0x01300200 - proc_pids (4 bytes * 32 = 128 bytes)
//   0x01300300 - proc_states (1 byte * 32 = 32 bytes) (0=free, 1=running, 2=sleeping, 3=stopped)
//   0x01300400 - proc_threads (4 bytes * 32 = 128 bytes) (thread count)

uint32_t MAX_PROCESSES = 32;
uint8_t* PROC_NAMES = 0x01300000;
uint32_t* PROC_PIDS = 0x01300200;
uint8_t* PROC_STATES = 0x01300300;
uint32_t* PROC_THREADS = 0x01300400;

uint8_t STATE_FREE = 0;
uint8_t STATE_RUNNING = 1;
uint8_t STATE_SLEEPING = 2;
uint8_t STATE_STOPPED = 3;

uint32_t next_pid = 1;
uint32_t current_task_index = 0;

void _process_copy_name(uint8_t* dst, uint8_t* src) {
    uint32_t i = 0;
    while (i < 16) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(src), "r"(i) : "%eax");
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(dst), "r"(i), "q"(c));
        if (c == 0) { break; }
        i += 1;
    }
}

uint32_t create_process(uint8_t* name) {
    uint32_t i = 0;
    while (i < MAX_PROCESSES) {
        uint8_t state = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(state) : "r"(PROC_STATES), "r"(i) : "%eax");
        if (state == STATE_FREE) {
            // Copy name
            uint32_t name_offset = i * 16;
            uint8_t* name_dst = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(name_dst) : "r"(PROC_NAMES), "r"(name_offset));
            _process_copy_name(name_dst, name);
            
            // Set PID
            uint32_t pid = next_pid;
            next_pid += 1;
            uint32_t pid_offset = i * 4;
            __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t" : : "r"(PROC_PIDS), "r"(pid_offset), "r"(pid) : "%eax");
            
            // Set state to running
            uint32_t running = STATE_RUNNING;
            __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(PROC_STATES), "r"(i), "q"(running));
            
            // 1 main thread
            uint32_t one_thread = 1;
            __asm__("movl %0, %%eax\n\t" "addl %1, %%eax\n\t" "movl %2, (%%eax)\n\t" : : "r"(PROC_THREADS), "r"(pid_offset), "r"(one_thread) : "%eax");
            
            return pid;
        }
        i += 1;
    }
    return 0; // Out of process slots
}

void init_processes() {
    uint32_t i = 0;
    while (i < MAX_PROCESSES) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(PROC_STATES), "r"(i));
        i += 1;
    }
    
    // Create the shell process as PID 1
    create_process("shell");
}

uint32_t get_pid_index(uint32_t pid) {
    uint32_t i = 0;
    while (i < MAX_PROCESSES) {
        uint8_t state = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(state) : "r"(PROC_STATES), "r"(i) : "%eax");
        if (state != STATE_FREE) {
            uint32_t curr_pid = 0;
            uint32_t pid_offset = i * 4;
            __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t" : "=r"(curr_pid) : "r"(PROC_PIDS), "r"(pid_offset) : "%eax");
            if (curr_pid == pid) {
                return i;
            }
        }
        i += 1;
    }
    return MAX_PROCESSES;
}

uint8_t kill_process(uint32_t pid) {
    if (pid == 1) { return 0; } // Don't kill init/shell
    uint32_t idx = get_pid_index(pid);
    if (idx == MAX_PROCESSES) { return 0; }
    
    // Set state to FREE
    uint32_t free_state = STATE_FREE;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(PROC_STATES), "r"(idx), "q"(free_state));
    return 1;
}

void print_ps() {
    print("PID  STATE    THREADS  COMMAND\n");
    uint32_t i = 0;
    while (i < MAX_PROCESSES) {
        uint8_t state = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(state) : "r"(PROC_STATES), "r"(i) : "%eax");
        if (state != STATE_FREE) {
            uint32_t curr_pid = 0;
            uint32_t pid_offset = i * 4;
            __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t" : "=r"(curr_pid) : "r"(PROC_PIDS), "r"(pid_offset) : "%eax");
            
            uint32_t threads = 0;
            __asm__("movl %1, %%eax\n\t" "addl %2, %%eax\n\t" "movl (%%eax), %0\n\t" : "=r"(threads) : "r"(PROC_THREADS), "r"(pid_offset) : "%eax");
            
            uint32_t name_offset = i * 16;
            uint8_t* name_ptr = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(name_ptr) : "r"(PROC_NAMES), "r"(name_offset));
            
            print_u32(curr_pid);
            if (curr_pid < 10) { print("    "); } else { print("   "); }
            
            if (state == STATE_RUNNING) { print("RUNNING  "); }
            else if (state == STATE_SLEEPING) { print("SLEEPING "); }
            else if (state == STATE_STOPPED) { print("STOPPED  "); }
            
            print_u32(threads);
            print("        ");
            print(name_ptr);
            print_char(10);
        }
        i += 1;
    }
}

// ─── Scheduler ────────────────────────────────────────────────────────────────
// A basic cooperative round-robin scheduler stub.
void schedule() {
    uint32_t start_index = current_task_index;
    uint32_t i = start_index + 1;
    if (i >= MAX_PROCESSES) { i = 0; }
    
    while (i != start_index) {
        uint8_t state = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(state) : "r"(PROC_STATES), "r"(i) : "%eax");
        
        if (state == STATE_RUNNING) {
            // Switch to this task
            current_task_index = i;
            
            // In a real preemptive kernel, we would swap context here (registers/stack).
            // For now, it's just updating tracking variables.
            return;
        }
        
        i += 1;
        if (i >= MAX_PROCESSES) { i = 0; }
    }
    // If no other task is running, continue current task
}



// ─── image_viewer.nux — Photo Viewer App Stub ─────────────────────────────────
//
// A program to view images inside the shell / desktop mode, supporting:
// - Zoom in / Zoom out
// - Panning (Right/Left/Top/Bottom)
//
// Requires:
// 1. Image decoding libraries (PNG, JPEG, BMP)
// 2. Framebuffer Graphics (VBE)

void open_image(uint8_t* filepath) {
    set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    print("--- Navi Photo Viewer ---\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    
    print("Opening image: ");
    print(filepath);
    print("\n");
    
    print("Controls: [+] Zoom In  [-] Zoom Out  [Arrow Keys] Pan Image\n");
    // Image decoding and framebuffer blitting logic would go here.
}







// ─── ramfs.nux — Mutable RAM File System ─────────────────────────────────────
//
// Layout in low memory:
//   0x01000000 — INITRD_BASE  (read-only initrd from GRUB)
//   0x01100000 — file_names   (16 bytes per file × MAX_FILES)
//   0x01110000 — file_data_ptrs (4 bytes per file, pointer to content)
//   0x01110100 — file_sizes   (4 bytes per file)
//   0x01120000 — file heap    (bump allocator for dynamic content)
//   0x01200000 — dir_names    (16 bytes per dir × MAX_DIRS)
//   0x01210000 — dir_file_counts (4 bytes per dir)
//
// All accesses done via inline asm pointer arithmetic to avoid needing
// struct support in the current Nux compiler.

uint32_t MAX_FILES = 32;
uint32_t MAX_DIRS = 16;
uint32_t NAME_LEN = 16;    // bytes reserved per filename

// Memory map
uint8_t* FILE_NAMES = 0x01100000;  // 16 bytes × 32 files = 512 bytes
uint32_t* FILE_DATA_PTRS = 0x01100200; // 4 bytes × 32 = 128 bytes
uint32_t* FILE_SIZES = 0x01100300; // 4 bytes × 32 = 128 bytes
uint8_t* FILE_FLAGS = 0x01100400; // 1 byte × 32 (0=empty, 1=used)
uint8_t* FILE_HEAP_BASE = 0x01120000;  // 512 KB for file content

uint8_t* DIR_NAMES = 0x01200000; // 16 bytes × 16 dirs
uint8_t* DIR_FLAGS = 0x01200100; // 1 byte × 16

uint8_t* file_heap_ptr = 0x01120000;
uint32_t file_count = 0;
uint32_t dir_count = 0;

// ─── Internal helpers ─────────────────────────────────────────────────────────

void _copy_name(uint8_t* dst, uint8_t* src) {
    uint32_t i = 0;
    while (i < NAME_LEN) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t"
            "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(src), "r"(i) : "%eax");
        __asm__("movb %b2, (%0,%1,1)\n\t"
            : : "r"(dst), "r"(i), "q"(c));
        if (c == 0) { break; }
        i += 1;
    }
}

uint8_t _names_equal(uint8_t* a, uint8_t* b) {
    uint32_t i = 0;
    while (i < NAME_LEN) {
        uint8_t ca = 0;
        uint8_t cb = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(ca) : "r"(a), "r"(i) : "%eax");
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(cb) : "r"(b), "r"(i) : "%eax");
        if (ca != cb) { return 0; }
        if (ca == 0) { return 1; }
        i += 1;
    }
    return 1;
}

// Bump-allocate n bytes from the file heap
uint8_t* _heap_alloc(uint32_t n) {
    uint32_t p = file_heap_ptr;
    uint32_t np = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
        : "=r"(np) : "r"(file_heap_ptr), "r"(n));
    file_heap_ptr = np;
    return p;
}

// Copy up to n bytes of src to dst
void _copy_bytes(uint8_t* dst, uint8_t* src, uint32_t n) {
    uint32_t i = 0;
    while (i < n) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(src), "r"(i) : "%eax");
        __asm__("movb %b2, (%0,%1,1)\n\t"
            : : "r"(dst), "r"(i), "q"(c));
        i += 1;
    }
    // Null-terminate
    __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(dst), "r"(n));
}

// ─── Initialisation ───────────────────────────────────────────────────────────

void init_ramfs() {
    // Clear the flag arrays (mark all slots empty)
    uint32_t i = 0;
    while (i < MAX_FILES) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(FILE_FLAGS), "r"(i));
        i += 1;
    }
    i = 0;
    while (i < MAX_DIRS) {
        __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(DIR_FLAGS), "r"(i));
        i += 1;
    }
    file_count = 0;
    dir_count = 0;
    file_heap_ptr = FILE_HEAP_BASE;
    // readme pre-population done at end of ramfs init (after create_file defined)
}

// ─── File Operations ──────────────────────────────────────────────────────────

// (All file helpers defined here, before init_ramfs calls create_file)

// Returns the file slot index or MAX_FILES if not found
uint32_t _find_file(uint8_t* name) {
    uint32_t i = 0;
    while (i < MAX_FILES) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(FILE_FLAGS), "r"(i) : "%eax");
        if (flag != 0) {
            // Compute name pointer: FILE_NAMES + i * NAME_LEN
            uint8_t* nptr = 0;
            uint32_t offset = i * NAME_LEN;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
                : "=r"(nptr) : "r"(FILE_NAMES), "r"(offset));
            if (_names_equal(nptr, name) != 0) {
                return i;
            }
        }
        i += 1;
    }
    return MAX_FILES;
}

// Find an empty file slot
uint32_t _alloc_file_slot() {
    uint32_t i = 0;
    while (i < MAX_FILES) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(FILE_FLAGS), "r"(i) : "%eax");
        if (flag == 0) { return i; }
        i += 1;
    }
    return MAX_FILES; // Full
}

// Create a file. Returns 1 on success, 0 on failure.
uint8_t create_file(uint8_t* name, uint8_t* data, uint32_t size) {
    // Check it doesn't already exist
    if (_find_file(name) != MAX_FILES) { return 0; }

    uint32_t slot = _alloc_file_slot();
    if (slot == MAX_FILES) { return 0; }

    // Copy name into FILE_NAMES[slot * NAME_LEN]
    uint32_t name_offset = slot * NAME_LEN;
    uint8_t* name_dst = 0;
    __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
        : "=r"(name_dst) : "r"(FILE_NAMES), "r"(name_offset));
    _copy_name(name_dst, name);

    // Allocate heap space and copy data
    uint32_t heap_ptr = _heap_alloc(size + 1);
    _copy_bytes(heap_ptr, data, size);

    // Store pointer into FILE_DATA_PTRS[slot] (4-byte aligned)
    uint32_t ptr_offset = slot * 4;
    __asm__("movl %0, %%eax\n\t"
        "addl %1, %%eax\n\t"
        "movl %2, (%%eax)\n\t"
        : : "r"(FILE_DATA_PTRS), "r"(ptr_offset), "r"(heap_ptr) : "%eax");

    // Store size
    __asm__("movl %0, %%eax\n\t"
        "addl %1, %%eax\n\t"
        "movl %2, (%%eax)\n\t"
        : : "r"(FILE_SIZES), "r"(ptr_offset), "r"(size) : "%eax");

    // Mark slot as used
    uint8_t one = 1;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(FILE_FLAGS), "r"(slot), "q"(one));

    file_count += 1;
    return 1;
}

// Overwrite file contents (append not supported yet — just replace)
uint8_t write_file(uint8_t* name, uint8_t* data, uint32_t size) {
    uint32_t slot = _find_file(name);
    if (slot == MAX_FILES) {
        // Create new
        return create_file(name, data, size);
    }
    // Allocate new heap space and update pointers
    uint32_t heap_ptr = _heap_alloc(size + 1);
    _copy_bytes(heap_ptr, data, size);

    uint32_t ptr_offset = slot * 4;
    __asm__("movl %0, %%eax\n\t"
        "addl %1, %%eax\n\t"
        "movl %2, (%%eax)\n\t"
        : : "r"(FILE_DATA_PTRS), "r"(ptr_offset), "r"(heap_ptr) : "%eax");
    __asm__("movl %0, %%eax\n\t"
        "addl %1, %%eax\n\t"
        "movl %2, (%%eax)\n\t"
        : : "r"(FILE_SIZES), "r"(ptr_offset), "r"(size) : "%eax");
    return 1;
}

// Delete a file. Returns 1 if deleted, 0 if not found.
uint8_t delete_file(uint8_t* name) {
    uint32_t slot = _find_file(name);
    if (slot == MAX_FILES) { return 0; }
    uint8_t zero = 0;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(FILE_FLAGS), "r"(slot), "q"(zero));
    file_count -= 1;
    return 1;
}

// Get data pointer for a file (returns 0 if not found)
uint8_t* get_file_data(uint8_t* name) {
    uint32_t slot = _find_file(name);
    if (slot == MAX_FILES) { return 0; }
    uint32_t ptr_offset = slot * 4;
    uint8_t* result = 0;
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl (%%eax), %0\n\t"
        : "=r"(result) : "r"(FILE_DATA_PTRS), "r"(ptr_offset) : "%eax");
    return result;
}

// Get size of a file
uint32_t get_file_size(uint8_t* name) {
    uint32_t slot = _find_file(name);
    if (slot == MAX_FILES) { return 0; }
    uint32_t ptr_offset = slot * 4;
    uint32_t sz = 0;
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl (%%eax), %0\n\t"
        : "=r"(sz) : "r"(FILE_SIZES), "r"(ptr_offset) : "%eax");
    return sz;
}

// Get nth file name (for ls / file manager listing)
uint8_t* get_file_name_at(uint32_t index) {
    // Skip empty slots
    uint32_t count = 0;
    uint32_t i = 0;
    while (i < MAX_FILES) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(FILE_FLAGS), "r"(i) : "%eax");
        if (flag != 0) {
            if (count == index) {
                uint32_t name_offset = i * NAME_LEN;
                uint8_t* nptr = 0;
                __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
                    : "=r"(nptr) : "r"(FILE_NAMES), "r"(name_offset));
                return nptr;
            }
            count += 1;
        }
        i += 1;
    }
    return 0;
}

// ─── Directory Operations ─────────────────────────────────────────────────────

uint32_t _find_dir(uint8_t* name) {
    uint32_t i = 0;
    while (i < MAX_DIRS) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(DIR_FLAGS), "r"(i) : "%eax");
        if (flag != 0) {
            uint8_t* nptr = 0;
            uint32_t offset = i * NAME_LEN;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
                : "=r"(nptr) : "r"(DIR_NAMES), "r"(offset));
            if (_names_equal(nptr, name) != 0) { return i; }
        }
        i += 1;
    }
    return MAX_DIRS;
}

uint8_t create_dir(uint8_t* name) {
    if (_find_dir(name) != MAX_DIRS) { return 0; } // already exists
    uint32_t i = 0;
    while (i < MAX_DIRS) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(DIR_FLAGS), "r"(i) : "%eax");
        if (flag == 0) {
            uint32_t offset = i * NAME_LEN;
            uint8_t* dst = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
                : "=r"(dst) : "r"(DIR_NAMES), "r"(offset));
            _copy_name(dst, name);
            uint8_t one = 1;
            __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(DIR_FLAGS), "r"(i), "q"(one));
            dir_count += 1;
            return 1;
        }
        i += 1;
    }
    return 0;
}

uint8_t delete_dir(uint8_t* name) {
    uint32_t slot = _find_dir(name);
    if (slot == MAX_DIRS) { return 0; }
    uint8_t zero = 0;
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(DIR_FLAGS), "r"(slot), "q"(zero));
    dir_count -= 1;
    return 1;
}

uint8_t* get_dir_name_at(uint32_t index) {
    uint32_t count = 0;
    uint32_t i = 0;
    while (i < MAX_DIRS) {
        uint8_t flag = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(flag) : "r"(DIR_FLAGS), "r"(i) : "%eax");
        if (flag != 0) {
            if (count == index) {
                uint32_t offset = i * NAME_LEN;
                uint8_t* nptr = 0;
                __asm__("movl %1, %0\n\t" "addl %2, %0\n\t"
                    : "=r"(nptr) : "r"(DIR_NAMES), "r"(offset));
                return nptr;
            }
            count += 1;
        }
        i += 1;
    }
    return 0;
}

// ─── Post-init (called after all file helpers are defined) ───────────────────

void populate_readme() {
    uint8_t* readme_content = "Welcome to Navi OS!\nPowered by the Nux language.\nType 'help' in the shell for commands.\n";
    create_file("readme.txt", readme_content, 87);
}






// ─── syscall.nux — System Call Interface ──────────────────────────────────────
//
// Implements INT 0x80 system call dispatching for application functionalities.
// 
// Syscall Numbers:
//   1 = sys_puts(str)
//   2 = sys_exit(code)
//   3 = sys_yield()

uint32_t syscall_dispatcher(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    if (num == 1) {
        // sys_puts
        print(arg1);
        return 0;
    }
    if (num == 2) {
        // sys_exit
        print("App exited with code ");
        print_u32(arg1);
        print_char(10);
        // In a real OS we'd kill the current thread here
        return 0;
    }
    if (num == 3) {
        // sys_yield
        // schedule_next(); (not implemented fully)
        return 0;
    }
    
    print("Unknown syscall: ");
    print_u32(num);
    print_char(10);
    return 0xFFFFFFFF;
}

void init_syscalls() {
    // _syscall_isr is defined in syscall_stub.s
    uint32_t isr_addr = 0;
    __asm__("movl $_syscall_isr, %0\n\t" : "=r"(isr_addr));
    
    // Register INT 0x80 in the IDT
    idt_set_gate(0x80, isr_addr);
    print("System calls (INT 0x80) initialized.\n");
}

// ─── User-space wrapper functions ─────────────────────────────────────────────

void puts(uint8_t* s) {
    uint32_t s_addr = s;
    __asm__("movl $1, %%eax\n\t"
        "movl %0, %%ebx\n\t"
        "int $0x80\n\t"
        : : "r"(s_addr) : "%eax", "%ebx");
}

void exit(uint32_t code) {
    __asm__("movl $2, %%eax\n\t"
        "movl %0, %%ebx\n\t"
        "int $0x80\n\t"
        : : "r"(code) : "%eax", "%ebx");
}

void yield() {
    __asm__("movl $3, %%eax\n\t"
        "int $0x80\n\t"
        : : : "%eax");
}






// --- mouse.nux -- PS/2 Mouse Driver with IntelliMouse scroll wheel support ----
//
// Supports:
//   - Standard PS/2 3-byte packets
//   - IntelliMouse 4-byte packets (scroll wheel)
//   - Left (bit0), Right (bit1), Middle (bit2) buttons
//   - Scroll: mouse_scroll_dir = 0 (none), 1 (up), 2 (down)
//
// PS/2 Y axis convention:
//   positive delta => mouse moved UP   => screen_y DECREASES
//   negative delta => mouse moved DOWN => screen_y INCREASES
//   (opposite of what you might expect — this is the hardware standard)

uint32_t mouse_x = 0;
uint32_t mouse_y = 0;
uint8_t mouse_buttons = 0;    // bit0=left, bit1=right, bit2=middle
uint8_t mouse_scroll_dir = 0;    // 0=none, 1=up, 2=down

uint8_t _mouse_cycle = 0;
uint8_t _mouse_has_scroll = 0;    // 1 if IntelliMouse Z enabled
uint8_t* _mouse_packet = 0x00600000;  // 4-byte buffer at fixed physical addr

// --- PS/2 controller helpers --------------------------------------------------
void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout > 0) {
            uint32_t status = inb(0x64);
            if ((status & 1) == 1) { return; }
            timeout -= 1;
        }
    } else {
        while (timeout > 0) {
            uint32_t status = inb(0x64);
            if ((status & 2) == 0) { return; }
            timeout -= 1;
        }
    }
}

void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

// --- Initialization -----------------------------------------------------------
void init_mouse() {
    _mouse_cycle      = 0;
    _mouse_has_scroll = 0;
    mouse_scroll_dir  = 0;

    // Start cursor at screen center
    mouse_x = fb_get_width()  / 2;
    mouse_y = fb_get_height() / 2;
    if (mouse_x == 0) { mouse_x = 512; }
    if (mouse_y == 0) { mouse_y = 384; }

    // Enable auxiliary mouse device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Enable mouse interrupts in the PS/2 command byte
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint32_t status = inb(0x60) | 2;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // Use default settings
    mouse_write(0xF6);
    mouse_read();  // ACK

    // --- Try to enable IntelliMouse scroll wheel ------------------------------
    // Magic sequence: set sample rates 200 -> 100 -> 80, then query ID.
    // If device replies with ID 0x03, 4-byte IntelliMouse packets are active.
    uint8_t r = 200;
    mouse_write(0xF3); mouse_read();
    mouse_write(r);    mouse_read();

    r = 100;
    mouse_write(0xF3); mouse_read();
    mouse_write(r);    mouse_read();

    r = 80;
    mouse_write(0xF3); mouse_read();
    mouse_write(r);    mouse_read();

    // Query device ID
    mouse_write(0xF2);
    mouse_read();  // ACK
    uint32_t dev_id = mouse_read();

    if (dev_id == 3) {
        _mouse_has_scroll = 1;
        print("Mouse: IntelliMouse (scroll wheel active)\n");
    } else {
        print("Mouse: Standard PS/2 (3-button)\n");
    }

    // Enable data reporting
    mouse_write(0xF4);
    mouse_read();  // ACK

    print("PS/2 Mouse initialized.\n");
}

// --- Public getters -----------------------------------------------------------
uint32_t get_mouse_x() { return mouse_x; }
uint32_t get_mouse_y() { return mouse_y; }
uint8_t get_mouse_buttons()  { return mouse_buttons; }
uint8_t get_mouse_scroll_dir()  { return mouse_scroll_dir; }

uint8_t mouse_left_pressed()  { return mouse_buttons & 1; }
uint8_t mouse_right_pressed()  { return (mouse_buttons >> 1) & 1; }
uint8_t mouse_middle_pressed()  { return (mouse_buttons >> 2) & 1; }

// --- Internal: decode a complete packet (defined FIRST so C sees it before use)
uint8_t _decode_packet() {
    uint32_t ptr = _mouse_packet;
    uint8_t b0 = 0; uint8_t b1 = 0; uint8_t b2 = 0;
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(b0) : "r"(ptr), "r"(0) : "%eax");
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(b1) : "r"(ptr), "r"(1) : "%eax");
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(b2) : "r"(ptr), "r"(2) : "%eax");

    // Discard overflow packets (data is unreliable)
    if ((b0 & 0xC0) != 0) { return 0; }

    // Button state: bit0=left, bit1=right, bit2=middle
    mouse_buttons = b0 & 0x07;

    // --- X axis ---------------------------------------------------------------
    // sign_x set (bit4 of b0) => dx is negative (moved left)
    uint8_t sign_x = (b0 >> 4) & 1;
    uint32_t dx = b1;

    if (sign_x != 0) {
        uint32_t mag_x = 0x100 - (dx & 0xFF);
        if (mag_x < mouse_x) { mouse_x -= mag_x; } else { mouse_x = 0; }
    } else {
        if (dx > 0) { mouse_x += dx; }
    }

    // --- Y axis (INVERTED from screen coordinates) ----------------------------
    // sign_y set (bit5 of b0) => dy is negative => mouse moved DOWN => screen_y++
    // sign_y clear             => dy is positive => mouse moved UP   => screen_y--
    uint8_t sign_y = (b0 >> 5) & 1;
    uint32_t dy = b2;

    if (sign_y != 0) {
        // Negative PS/2 Y = down = increase screen Y
        uint32_t mag_y = 0x100 - (dy & 0xFF);
        uint32_t h = fb_get_height();
        mouse_y += mag_y;
        if (mouse_y >= h) { mouse_y = h - 1; }
    } else {
        // Positive PS/2 Y = up = decrease screen Y
        if (dy > 0) {
            if (dy < mouse_y) { mouse_y -= dy; } else { mouse_y = 0; }
        }
    }

    // Clamp X
    uint32_t w = fb_get_width();
    if (mouse_x >= w) { mouse_x = w - 1; }

    // --- Scroll wheel (4th byte) ----------------------------------------------
    mouse_scroll_dir = 0;
    if (_mouse_has_scroll != 0) {
        uint8_t b3 = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(b3) : "r"(ptr), "r"(3) : "%eax");
        uint8_t z = b3 & 0x0F;
        if (z != 0) {
            if ((z & 0x08) != 0) {
                // bit3 set => negative (two's complement nibble) => scroll DOWN
                mouse_scroll_dir = 2;
            } else {
                // positive nibble => scroll UP
                mouse_scroll_dir = 1;
            }
        }
    }

    return 1;
}

// --- Process one incoming PS/2 byte -------------------------------------------
// Returns 1 when a complete packet is processed, 0 while accumulating.
uint8_t handle_mouse_byte(uint8_t b) {
    // Byte 0: status byte — must have bit3 set (always-1), use as sync guard
    if (_mouse_cycle == 0) {
        if ((b & 0x08) == 0) { return 0; }   // Resync: not first byte
        uint32_t ptr0 = _mouse_packet;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(ptr0), "r"(0), "q"(b));
        _mouse_cycle = 1;
        return 0;
    }

    // Byte 1: X delta
    if (_mouse_cycle == 1) {
        uint32_t ptr1 = _mouse_packet;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(ptr1), "r"(1), "q"(b));
        _mouse_cycle = 2;
        return 0;
    }

    // Byte 2: Y delta
    if (_mouse_cycle == 2) {
        uint32_t ptr2 = _mouse_packet;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(ptr2), "r"(2), "q"(b));
        if (_mouse_has_scroll != 0) {
            _mouse_cycle = 3;
            return 0;
        }
        _mouse_cycle = 0;
        return _decode_packet();
    }

    // Byte 3: Scroll wheel Z (IntelliMouse only)
    if (_mouse_cycle == 3) {
        uint32_t ptr3 = _mouse_packet;
        __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(ptr3), "r"(3), "q"(b));
        _mouse_cycle = 0;
        return _decode_packet();
    }

    _mouse_cycle = 0;
    return 0;
}






// --- gui_vga.nux -- VGA Text Buffer → Framebuffer Bridge ----------------------
//
// When a text-mode app (shell, fm, editor) runs inside a GUI window, this
// module reads the VGA text buffer at 0xB8000 and renders each changed cell
// into the framebuffer at the window's pixel offset.
//
// Shadow buffer at 0x01930000 stores the last-rendered state so only changed
// cells get redrawn (dirty-cell optimisation).
//
// Colour palette: Catppuccin Mocha — matches the desktop theme exactly.

// ─── Catppuccin Mocha palette (32-bit 0xRRGGBB) ─────────────────────────────
uint32_t VGA_C_BASE = 0x1E1E2E;  // 0  – Black
uint32_t VGA_C_BLUE = 0x89B4FA;  // 1  – Blue
uint32_t VGA_C_GREEN = 0xA6E3A1;  // 2  – Green
uint32_t VGA_C_TEAL = 0x94E2D5;  // 3  – Cyan / Teal
uint32_t VGA_C_RED = 0xF38BA8;  // 4  – Red
uint32_t VGA_C_MAUVE = 0xCBA6F7;  // 5  – Magenta
uint32_t VGA_C_YELLOW = 0xF9E2AF;  // 6  – Brown / Yellow
uint32_t VGA_C_LGREY = 0xBAC2DE;  // 7  – Light Grey
uint32_t VGA_C_DGREY = 0x6C7086;  // 8  – Dark Grey
uint32_t VGA_C_LBLUE = 0x74C7EC;  // 9  – Light Blue
uint32_t VGA_C_LGREEN = 0xA6E3A1;  // 10 – Light Green
uint32_t VGA_C_LCYAN = 0x89DCEB;  // 11 – Light Cyan
uint32_t VGA_C_LRED = 0xF38BA8;  // 12 – Light Red
uint32_t VGA_C_LMAUVE = 0xCBA6F7;  // 13 – Light Magenta
uint32_t VGA_C_LYELLOW = 0xF9E2AF;  // 14 – Yellow (bright)
uint32_t VGA_C_TEXT = 0xCDD6F4;  // 15 – White / Main text

// ─── Module state ────────────────────────────────────────────────────────────
uint32_t VGA_WIN_X = 0;
uint32_t VGA_WIN_Y = 0;
uint8_t VGA_ACTIVE = 0;

// ─── Map a 4-bit VGA colour index to its GUI colour ──────────────────────────
uint32_t vga_color_to_gui(uint8_t c) {
    uint8_t idx = c & 0x0F;
    if (idx ==  0) { return VGA_C_BASE; }
    if (idx ==  1) { return VGA_C_BLUE; }
    if (idx ==  2) { return VGA_C_GREEN; }
    if (idx ==  3) { return VGA_C_TEAL; }
    if (idx ==  4) { return VGA_C_RED; }
    if (idx ==  5) { return VGA_C_MAUVE; }
    if (idx ==  6) { return VGA_C_YELLOW; }
    if (idx ==  7) { return VGA_C_LGREY; }
    if (idx ==  8) { return VGA_C_DGREY; }
    if (idx ==  9) { return VGA_C_LBLUE; }
    if (idx == 10) { return VGA_C_LGREEN; }
    if (idx == 11) { return VGA_C_LCYAN; }
    if (idx == 12) { return VGA_C_LRED; }
    if (idx == 13) { return VGA_C_LMAUVE; }
    if (idx == 14) { return VGA_C_LYELLOW; }
    return VGA_C_TEXT;
}

// ─── Start VGA emulation, anchored at a window's top-left ────────────────────
void gui_vga_start(uint32_t win_x, uint32_t win_y) {
    VGA_WIN_X  = win_x;
    VGA_WIN_Y  = win_y;
    VGA_ACTIVE = 1;

    // Invalidate shadow buffer so first sync does a full redraw.
    // We write 0xFF bytes (impossible VGA value) to every shadow cell.
    uint32_t shd = 0x01930000;
    uint32_t i = 0;
    while (i < 4000) {         // 80 cols x 25 rows x 2 bytes = 4000
        __asm__("movb $0xFF, (%0,%1,1)\n\t" : : "r"(shd), "r"(i));
        i += 1;
    }
}

void gui_vga_stop() {
    VGA_ACTIVE = 0;
}

uint8_t gui_vga_is_active() {
    return VGA_ACTIVE;
}

// ─── Sync changed VGA text cells to the framebuffer ──────────────────────────
// Call this every time something may have printed text.
// Returns 1 if any cell changed, 0 otherwise.
uint8_t gui_sync_vga() {
    if (VGA_ACTIVE == 0) { return 0; }

    // Text-mode content area starts 8 px inside window and 28 px below title.
    uint32_t base_x = VGA_WIN_X + 8;
    uint32_t base_y = VGA_WIN_Y + 28;

    uint32_t vga_base = 0xB8000;
    uint32_t shd_base = 0x01930000;

    uint8_t changed = 0;
    uint32_t row = 0;
    while (row < 25) {
        uint32_t col = 0;
        while (col < 80) {
            // Byte offset = row*160 + col*2  (done with Nux arithmetic)
            uint32_t off = row * 160;
            off += col * 2;

            // Read VGA cell (character | attribute)
            uint8_t ch = 0;
            uint8_t attr = 0;
            __asm__("movb (%1,%2,1), %0\n\t" : "=r"(ch)   : "r"(vga_base), "r"(off));
            uint32_t off1 = off + 1;
            __asm__("movb (%1,%2,1), %0\n\t" : "=r"(attr) : "r"(vga_base), "r"(off1));

            // Read shadow cell
            uint8_t sch = 0;
            uint8_t sattr = 0;
            __asm__("movb (%1,%2,1), %0\n\t" : "=r"(sch)   : "r"(shd_base), "r"(off));
            __asm__("movb (%1,%2,1), %0\n\t" : "=r"(sattr) : "r"(shd_base), "r"(off1));

            if (ch != sch || attr != sattr) {
                changed = 1;

                // Update shadow
                __asm__("movb %2, (%0,%1,1)\n\t" : : "r"(shd_base), "r"(off),  "q"(ch));
                __asm__("movb %2, (%0,%1,1)\n\t" : : "r"(shd_base), "r"(off1), "q"(attr));

                // Decode colours (pure Nux – no asm needed)
                uint8_t fg_idx = attr & 0x0F;
                uint8_t bg_idx = (attr >> 4) & 0x0F;
                uint32_t fg = vga_color_to_gui(fg_idx);
                uint32_t bg = vga_color_to_gui(bg_idx);

                // Pixel coords for this cell (8x16 font)
                uint32_t px = base_x + col * 8;
                uint32_t py = base_y + row * 16;

                // Fill background, then draw character
                fb_fill_rect(px, py, 8, 16, bg);
                if (ch != 32 && ch != 0) {
                    font_draw_char(ch, px, py, fg, bg);
                }
            }

            col += 1;
        }
        row += 1;
    }

    if (changed != 0) {
        fb_swap_buffers();
    }
    return changed;
}









// --- gui.nux -- Graphical Desktop UI ------------------------------------------
//
// Color palette: Catppuccin Mocha
uint32_t C_BASE = 0x1E1E2E;
uint32_t C_MANTLE = 0x181825;
uint32_t C_SURFACE = 0x313244;
uint32_t C_OVERLAY = 0x6C7086;
uint32_t C_TEXT = 0xCDD6F4;
uint32_t C_BLUE = 0x89B4FA;
uint32_t C_GREEN = 0xA6E3A1;
uint32_t C_RED = 0xF38BA8;
uint32_t C_YELLOW = 0xF9E2AF;
uint32_t C_MAUVE = 0xCBA6F7;
uint32_t C_WHITE = 0xFFFFFF;

uint32_t TASKBAR_H = 28;
uint32_t DOCK_H = 48;

// Clock string buffer: "HH:MM:SS  DD Mon YYYY" + null = 22 bytes at fixed addr
uint8_t* _CLK_BUF = 0x01910000;

// Month name table at fixed addr (3 chars + space per month = 4*12 = 48 bytes)
uint8_t* _MON_TBL = 0x01910040;
uint8_t _MON_INIT = 0;

// Day-of-week table (3 chars + null = 4*7 = 28 bytes)
uint8_t* _DOW_TBL = 0x01910080;

// --- Asset loaders -----------------------------------------------------------
uint32_t _get_cursor() { uint32_t p = 0; __asm__("movl $asset_cursor, %0\n\t"       : "=r"(p)); return p; }
uint32_t _get_close() { uint32_t p = 0; __asm__("movl $asset_icon_close, %0\n\t"   : "=r"(p)); return p; }
uint32_t _get_min() { uint32_t p = 0; __asm__("movl $asset_icon_minimize, %0\n\t": "=r"(p)); return p; }
uint32_t _get_max() { uint32_t p = 0; __asm__("movl $asset_icon_maximize, %0\n\t": "=r"(p)); return p; }
uint32_t _get_term() { uint32_t p = 0; __asm__("movl $asset_icon_terminal, %0\n\t": "=r"(p)); return p; }
uint32_t _get_file() { uint32_t p = 0; __asm__("movl $asset_icon_file, %0\n\t"    : "=r"(p)); return p; }
uint32_t _get_set() { uint32_t p = 0; __asm__("movl $asset_icon_settings, %0\n\t": "=r"(p)); return p; }

// --- Write a single byte to a buffer -----------------------------------------
void _buf_putb(uint8_t* buf, uint32_t idx, uint8_t c) {
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buf), "r"(idx), "q"(c));
}
uint8_t _buf_getb(uint8_t* buf, uint32_t idx) {
    uint8_t c = 0;
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(buf), "r"(idx) : "%eax");
    return c;
}

// --- Write 3-char string into buffer at offset --------------------------------
void _buf_puts3(uint8_t* buf, uint32_t off, uint8_t a, uint8_t b, uint8_t c) {
    _buf_putb(buf, off,       a);
    uint32_t o1 = 0; __asm__("movl %1, %0\n\t" "addl $1, %0\n\t" : "=r"(o1) : "r"(off));
    uint32_t o2 = 0; __asm__("movl %1, %0\n\t" "addl $2, %0\n\t" : "=r"(o2) : "r"(off));
    _buf_putb(buf, o1, b);
    _buf_putb(buf, o2, c);
}

// --- Init month & day-of-week name tables ------------------------------------
void _init_month_table() {
    if (_MON_INIT != 0) { return; }
    _MON_INIT = 1;

    // Months 1-12, each 4 bytes (3 chars + space), stored at offsets 0,4,8,...44
    _buf_puts3(_MON_TBL,  0, 74, 97, 110);  // Jan
    _buf_puts3(_MON_TBL,  4, 70,101, 98);   // Feb
    _buf_puts3(_MON_TBL,  8, 77, 97,114);   // Mar
    _buf_puts3(_MON_TBL, 12, 65,112,114);   // Apr
    _buf_puts3(_MON_TBL, 16, 77, 97,121);   // May
    _buf_puts3(_MON_TBL, 20, 74,117,110);   // Jun
    _buf_puts3(_MON_TBL, 24, 74,117,108);   // Jul
    _buf_puts3(_MON_TBL, 28, 65,117,103);   // Aug
    _buf_puts3(_MON_TBL, 32, 83,101,112);   // Sep
    _buf_puts3(_MON_TBL, 36, 79, 99,116);   // Oct
    _buf_puts3(_MON_TBL, 40, 78,111,118);   // Nov
    _buf_puts3(_MON_TBL, 44, 68,101, 99);   // Dec

    // Day of week: RTC wday 1=Sun..7=Sat, each 3 chars at offsets 0,3,6,...18
    _buf_puts3(_DOW_TBL,  0, 83,117,110);   // Sun
    _buf_puts3(_DOW_TBL,  3, 77,111,110);   // Mon
    _buf_puts3(_DOW_TBL,  6, 84,117,101);   // Tue
    _buf_puts3(_DOW_TBL,  9, 87,101,100);   // Wed
    _buf_puts3(_DOW_TBL, 12, 84,104,117);   // Thu
    _buf_puts3(_DOW_TBL, 15, 70,114,105);   // Fri
    _buf_puts3(_DOW_TBL, 18, 83, 97,116);   // Sat
}

// --- Build clock string into _CLK_BUF: "HH:MM:SS  DD Mon YYYY" -------------
// Format: [0..1]=HH [2]=: [3..4]=MM [5]=: [6..7]=SS [8..9]=sp
//         [10..11]=DD [12]=sp [13..15]=Mon [16]=sp [17..20]=YYYY [21]=\0
void _build_clock_str() {
    _init_month_table();
    rtc_read_time();

    uint32_t h = rtc_get_hour();
    uint32_t mn = rtc_get_min();
    uint32_t s = rtc_get_sec();
    uint32_t d = rtc_get_day();
    uint32_t mo = rtc_get_mon();
    uint32_t yr = rtc_get_year();
    uint32_t ct = rtc_get_cent();
    uint32_t wd = rtc_get_wday();

    uint32_t buf = _CLK_BUF;

    // HH:MM:SS
    rtc_write_digits(h,  buf, 0);
    _buf_putb(buf, 2, 58);  // ':'
    rtc_write_digits(mn, buf, 3);
    _buf_putb(buf, 5, 58);  // ':'
    rtc_write_digits(s,  buf, 6);

    // Two spaces
    _buf_putb(buf, 8, 32);
    _buf_putb(buf, 9, 32);

    // DD
    rtc_write_digits(d, buf, 10);
    _buf_putb(buf, 12, 32);  // space

    // Mon (3 chars from _MON_TBL)
    uint32_t mo_safe = mo;
    if (mo_safe < 1)  { mo_safe = 1; }
    if (mo_safe > 12) { mo_safe = 12; }
    uint32_t mo_off = 0;
    __asm__("movl %1, %0\n\t" "subl $1, %0\n\t" "imull $4, %0\n\t" : "=r"(mo_off) : "r"(mo_safe));
    uint32_t ma = _buf_getb(_MON_TBL, mo_off);
    uint32_t mo_off1 = 0; __asm__("movl %1, %0\n\t" "addl $1, %0\n\t" : "=r"(mo_off1) : "r"(mo_off));
    uint32_t mo_off2 = 0; __asm__("movl %1, %0\n\t" "addl $2, %0\n\t" : "=r"(mo_off2) : "r"(mo_off));
    uint32_t mb = _buf_getb(_MON_TBL, mo_off1);
    uint32_t mc = _buf_getb(_MON_TBL, mo_off2);
    _buf_putb(buf, 13, ma);
    _buf_putb(buf, 14, mb);
    _buf_putb(buf, 15, mc);
    _buf_putb(buf, 16, 32);  // space

    // YYYY = cent * 100 + year
    // Compute century digits
    rtc_write_digits(ct, buf, 17);
    rtc_write_digits(yr, buf, 19);

    _buf_putb(buf, 21, 0);  // null terminator
}

// --- Draw top taskbar with live RTC clock ------------------------------------
void gui_draw_taskbar() {
    uint32_t w = fb_get_width();

    fb_fill_rect(0, 0, w, TASKBAR_H, C_MANTLE);
    fb_draw_hline(0, TASKBAR_H - 1, w, C_SURFACE);
    font_draw_string("Navi OS", 8, 6, C_MAUVE, C_MANTLE);
    fb_draw_vline(80, 4, TASKBAR_H - 8, C_SURFACE);

    // Build and draw live clock
    _build_clock_str();
    // Clock string is 21 chars * 8px wide = 168px; place at right edge
    uint32_t clk_x = 0;
    __asm__("movl %1, %0\n\t" "subl $170, %0\n\t" : "=r"(clk_x) : "r"(w));
    font_draw_string(_CLK_BUF, clk_x, 6, C_TEXT, C_MANTLE);
}

// --- Draw bottom dock ---------------------------------------------------------
void gui_draw_dock() {
    uint32_t w = fb_get_width();
    uint32_t h = fb_get_height();
    uint32_t dock_y = 0;
    __asm__("movl %1, %0\n\t" "subl %2, %0\n\t" : "=r"(dock_y) : "r"(h), "r"(DOCK_H));

    fb_fill_rect(0, dock_y, w, DOCK_H, C_MANTLE);
    fb_draw_hline(0, dock_y, w, C_SURFACE);

    uint32_t icon_y = dock_y;
    fb_draw_image(8,   icon_y, _get_term());
    fb_draw_image(72,  icon_y, _get_file());
    fb_draw_image(136, icon_y, _get_set());

    // Icon labels
    uint32_t label_y = 0;
    __asm__("movl %1, %0\n\t" "addl $36, %0\n\t" : "=r"(label_y) : "r"(dock_y));
    font_draw_string("Terminal", 4,   label_y, C_OVERLAY, C_MANTLE);
    font_draw_string("Files",    68,  label_y, C_OVERLAY, C_MANTLE);
    font_draw_string("Settings", 128, label_y, C_OVERLAY, C_MANTLE);
}

// --- Draw wallpaper (gradient) -----------------------------------------------
void gui_draw_wallpaper() {
    uint32_t w = fb_get_width();
    uint32_t h = fb_get_height();
    uint32_t content_y = TASKBAR_H;
    uint32_t content_h = 0;
    __asm__("movl %1, %0\n\t" "subl %2, %0\n\t" "subl %3, %0\n\t"
        : "=r"(content_h) : "r"(h), "r"(TASKBAR_H), "r"(DOCK_H));

    uint32_t row = 0;
    while (row < content_h) {
        uint32_t blend = (row * 40) / content_h;
        uint32_t r = 0x1E + blend;
        uint32_t g = 0x1E + blend;
        uint32_t b = 0x2E + (blend * 2);
        if (r > 0xFF) { r = 0xFF; }
        if (g > 0xFF) { g = 0xFF; }
        if (b > 0xFF) { b = 0xFF; }
        uint32_t color = 0;
        __asm__("movl %1, %0\n\t" "shll $16, %0\n\t" "orl %2, %0\n\t" "shll $8, %0\n\t" "orl %3, %0\n\t"
            : "=r"(color) : "r"(r), "r"(g), "r"(b));
        fb_draw_hline(0, content_y + row, w, color);
        row += 1;
    }
}

// --- Draw a window frame ------------------------------------------------------
void gui_draw_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t* title) {
    fb_fill_rect(x, y, w, h, C_BASE);
    fb_fill_rect(x, y, w, 24, C_SURFACE);
    fb_draw_hline(x, y + 24, w, C_OVERLAY);
    fb_draw_rect(x, y, w, h, C_SURFACE);
    font_draw_string(title, x + 8, y + 4, C_TEXT, C_SURFACE);
    fb_draw_image(x + w - 20, y + 4, _get_close());
    fb_draw_image(x + w - 40, y + 4, _get_max());
    fb_draw_image(x + w - 60, y + 4, _get_min());
}

// --- Draw a single row in a terminal output area -----------------------------
// row_buf: ptr to null-terminated string, tx/ty: top-left of that row
void gui_draw_term_row(uint8_t* row_buf, uint32_t tx, uint32_t ty, uint32_t fg) {
    font_draw_string(row_buf, tx, ty, fg, C_MANTLE);
}

// --- GUI Terminal App (interactive) ------------------------------------------
// Output buffer: 30 rows x 80 cols chars at 0x01920000
uint32_t TERM_ROWS = 26;
uint32_t TERM_COLS = 80;
uint8_t* TERM_BUF = 0x01920000;   // 30*80 = 2400 bytes
uint8_t* TERM_INPUT = 0x01920A00;   // 128-byte input line buffer
uint32_t TERM_CURSOR_ROW = 0;
uint32_t TERM_CURSOR_COL = 0;

// Clear terminal buffer
void gui_term_clear() {
    uint32_t i = 0;
    uint32_t total = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(total) : "r"(TERM_ROWS), "r"(TERM_COLS));
    while (i < total) {
        _buf_putb(TERM_BUF, i, 32);  // space
        i += 1;
    }
    TERM_CURSOR_ROW = 0;
    TERM_CURSOR_COL = 0;
}

// Scroll terminal up one line
void gui_term_scroll() {
    uint32_t row = 0;
    while (row < TERM_ROWS - 1) {
        uint32_t dst_off = 0;
        uint32_t src_off = 0;
        __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(dst_off) : "r"(row), "r"(TERM_COLS));
        uint32_t next_row = 0;
        __asm__("movl %1, %0\n\t" "addl $1, %0\n\t" : "=r"(next_row) : "r"(row));
        __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(src_off) : "r"(next_row), "r"(TERM_COLS));
        uint32_t col = 0;
        while (col < TERM_COLS) {
            uint32_t src_idx = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(src_idx) : "r"(src_off), "r"(col));
            uint32_t dst_idx = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(dst_idx) : "r"(dst_off), "r"(col));
            uint32_t c = _buf_getb(TERM_BUF, src_idx);
            _buf_putb(TERM_BUF, dst_idx, c);
            col += 1;
        }
        row += 1;
    }
    // Clear last row
    uint32_t last_off = 0;
    __asm__("movl %1, %0\n\t" "subl $1, %0\n\t" "imull %2, %0\n\t" : "=r"(last_off) : "r"(TERM_ROWS), "r"(TERM_COLS));
    uint32_t col = 0;
    while (col < TERM_COLS) {
        uint32_t idx = 0;
        __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(idx) : "r"(last_off), "r"(col));
        _buf_putb(TERM_BUF, idx, 32);
        col += 1;
    }
}

// Print a char to the terminal buffer (handles newline + scroll)
void gui_term_putch(uint8_t c) {
    if (c == 10 || c == 13) {  // newline
        // Pad rest of row with spaces
        while (TERM_CURSOR_COL < TERM_COLS) {
            uint32_t idx = 0;
            __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
                : "=r"(idx) : "r"(TERM_CURSOR_ROW), "r"(TERM_COLS), "r"(TERM_CURSOR_COL));
            _buf_putb(TERM_BUF, idx, 32);
            TERM_CURSOR_COL += 1;
        }
        TERM_CURSOR_COL = 0;
        TERM_CURSOR_ROW += 1;
        if (TERM_CURSOR_ROW >= TERM_ROWS) {
            gui_term_scroll();
            TERM_CURSOR_ROW = TERM_ROWS - 1;
        }
        return;
    }
    if (c == 8) {  // backspace
        if (TERM_CURSOR_COL > 0) {
            TERM_CURSOR_COL -= 1;
            uint32_t idx = 0;
            __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
                : "=r"(idx) : "r"(TERM_CURSOR_ROW), "r"(TERM_COLS), "r"(TERM_CURSOR_COL));
            _buf_putb(TERM_BUF, idx, 32);
        }
        return;
    }
    if (TERM_CURSOR_COL >= TERM_COLS) {
        TERM_CURSOR_COL = 0;
        TERM_CURSOR_ROW += 1;
        if (TERM_CURSOR_ROW >= TERM_ROWS) {
            gui_term_scroll();
            TERM_CURSOR_ROW = TERM_ROWS - 1;
        }
    }
    uint32_t idx = 0;
    __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" "addl %3, %0\n\t"
        : "=r"(idx) : "r"(TERM_CURSOR_ROW), "r"(TERM_COLS), "r"(TERM_CURSOR_COL));
    _buf_putb(TERM_BUF, idx, c);
    TERM_CURSOR_COL += 1;
}

// Print a null-terminated string to terminal buffer
void gui_term_print(uint8_t* s) {
    uint32_t i = 0;
    while (true) {
        uint32_t c = _buf_getb(s, i);
        if (c == 0) { break; }
        gui_term_putch(c);
        i += 1;
    }
}

// Print a u32 number to terminal
void gui_term_print_u32(uint32_t v) {
    if (v == 0) { gui_term_putch(48); return; }
    // Build digits in reverse into a small stack buffer at 0x01920C00
    uint8_t* dbuf = 0x01920C00;
    uint32_t len = 0;
    uint32_t n = v;
    while (n > 0) {
        uint32_t digit = n - ((n / 10) * 10);
        uint8_t dc = (digit) + 48;
        _buf_putb(dbuf, len, dc);
        len += 1;
        n = n / 10;
    }
    // Print in reverse order
    uint32_t i = len;
    while (i > 0) {
        i -= 1;
        uint32_t c = _buf_getb(dbuf, i);
        gui_term_putch(c);
    }
}

// Render the terminal buffer to the screen (inside the window)
void gui_term_redraw(uint32_t win_x, uint32_t win_y, uint32_t win_w, uint32_t win_h) {
    // Content area starts at win_y + 25 (after title bar)
    uint32_t cx = 0;
    __asm__("movl %1, %0\n\t" "addl $6, %0\n\t" : "=r"(cx) : "r"(win_x));
    uint32_t cy = 0;
    __asm__("movl %1, %0\n\t" "addl $28, %0\n\t" : "=r"(cy) : "r"(win_y));

    fb_fill_rect(win_x + 1, win_y + 25, win_w - 2, win_h - 26, C_MANTLE);

    uint32_t row = 0;
    while (row < TERM_ROWS) {
        uint32_t row_off = 0;
        __asm__("movl %1, %0\n\t" "imull %2, %0\n\t" : "=r"(row_off) : "r"(row), "r"(TERM_COLS));

        uint32_t ty = 0;
        __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(ty) : "r"(cy), "r"(row));
        ty += row;  // add row * 0 but we need row * FONT_HEIGHT
        // ty = cy + row * 16
        __asm__("movl %1, %0\n\t" "imull $16, %1\n\t" "addl %1, %0\n\t"
            : "=r"(ty) : "r"(row), "r"(cy));

        uint32_t col = 0;
        while (col < TERM_COLS) {
            uint32_t idx = 0;
            __asm__("movl %1, %0\n\t" "addl %2, %0\n\t" : "=r"(idx) : "r"(row_off), "r"(col));
            uint32_t c = _buf_getb(TERM_BUF, idx);

            uint32_t tx = 0;
            __asm__("movl %1, %0\n\t" "imull $8, %0\n\t" "addl %2, %0\n\t" : "=r"(tx) : "r"(col), "r"(cx));

            uint32_t fg_color = C_TEXT;
            if (row == TERM_CURSOR_ROW && col == TERM_CURSOR_COL) {
                fg_color = C_BASE;
                fb_fill_rect(tx, ty, 8, 16, C_TEXT);  // block cursor
            }
            font_draw_char(c, tx, ty, fg_color, C_MANTLE);
            col += 1;
        }
        row += 1;
    }
}

// --- Draw initial (static) terminal window for desktop render -----------------
void gui_draw_terminal() {
    uint32_t w = fb_get_width();
    uint32_t h = fb_get_height();
    uint32_t win_x = 40;
    uint32_t win_y = 40;
    uint32_t win_w = 0;
    uint32_t win_h = 0;
    __asm__("movl %1, %0\n\t" "subl $80, %0\n\t"  : "=r"(win_w) : "r"(w));
    __asm__("movl %1, %0\n\t" "subl $120, %0\n\t" : "=r"(win_h) : "r"(h));

    gui_draw_window(win_x, win_y, win_w, win_h, "Terminal -- Navi OS");
    fb_fill_rect(win_x + 1, win_y + 25, win_w - 2, win_h - 26, C_MANTLE);
    font_draw_string("Welcome to Navi OS", win_x + 8, win_y + 32, C_GREEN, C_MANTLE);
    font_draw_string("Click the terminal icon to open an interactive shell.", win_x + 8, win_y + 50, C_TEXT, C_MANTLE);
    font_draw_string("root@navi:/> _", win_x + 8, win_y + 76, C_BLUE, C_MANTLE);
}

// --- Full desktop render ------------------------------------------------------
void gui_render_desktop() {
    if (fb_is_active() == 0) {
        print("[GUI] No framebuffer. Boot with VBE mode.\n");
        return;
    }

    fb_clear(C_BASE);
    gui_draw_wallpaper();
    gui_draw_taskbar();
    gui_draw_dock();
    gui_draw_terminal();

    fb_swap_buffers();

    uint32_t mx = get_mouse_x();
    uint32_t my = get_mouse_y();
    fb_draw_image_vram(mx, my, _get_cursor());
}

// --- Fast mouse update --------------------------------------------------------
void gui_update_mouse(uint32_t ox, uint32_t oy, uint32_t nx, uint32_t ny) {
    fb_restore_rect(ox, oy, 24, 24);
    fb_draw_image_vram(nx, ny, _get_cursor());
}

// --- Initialize GUI subsystem -------------------------------------------------
void init_gui() {
    init_font();
    gui_term_clear();
    gui_term_print("Navi OS GUI Terminal\n");
    gui_term_print("Type commands, press Enter.\n");
    gui_term_print("Type 'exit' to return to desktop.\n");
    gui_term_print("root@navi:/> ");
    print("[GUI] Font + terminal buffer initialized.\n");
}








// ─── keyboard.nux — Full PS/2 Keyboard Driver ─────────────────────────────────
//
// Supports: full QWERTY, Shift, CapsLock, Ctrl, Alt, arrow keys, F-keys, etc.
// Key events are returned as a u32 packed value:
//   bits [7:0]  = ASCII char (0 if non-printable)
//   bits [15:8] = special key code (KEY_* constants below)
//   bits [23:16] = modifier flags (MOD_*)

// ─── Modifier Flags ───────────────────────────────────────────────────────────
uint8_t MOD_SHIFT = 1;
uint8_t MOD_CTRL = 2;
uint8_t MOD_ALT = 4;
uint8_t MOD_CAPS = 8;

// ─── Special Key Codes ────────────────────────────────────────────────────────
uint8_t KEY_NONE = 0;
uint8_t KEY_ENTER = 1;
uint8_t KEY_BACKSPACE = 2;
uint8_t KEY_TAB = 3;
uint8_t KEY_ESC = 4;
uint8_t KEY_UP = 5;
uint8_t KEY_DOWN = 6;
uint8_t KEY_LEFT = 7;
uint8_t KEY_RIGHT = 8;
uint8_t KEY_HOME = 9;
uint8_t KEY_END = 10;
uint8_t KEY_PGUP = 11;
uint8_t KEY_PGDN = 12;
uint8_t KEY_DEL = 13;
uint8_t KEY_INSERT = 14;
uint8_t KEY_F1 = 20;
uint8_t KEY_F2 = 21;
uint8_t KEY_F3 = 22;
uint8_t KEY_F4 = 23;
uint8_t KEY_F5 = 24;
uint8_t KEY_F6 = 25;
uint8_t KEY_F7 = 26;
uint8_t KEY_F8 = 27;
uint8_t KEY_F9 = 28;
uint8_t KEY_F10 = 29;
uint8_t KEY_F11 = 30;
uint8_t KEY_F12 = 31;

// ─── Internal Driver State ────────────────────────────────────────────────────
uint8_t shift_down = 0;
uint8_t ctrl_down = 0;
uint8_t alt_down = 0;
uint8_t caps_lock = 0;
uint8_t extended = 0;    // Set when 0xE0 prefix is seen

// ─── Scancode → ASCII tables ─────────────────────────────────────────────────
// These are flat arrays at fixed low-memory addresses.
// Layout: unshifted[128] at 0x9000, shifted[128] at 0x9080

void init_keymaps() {
    // We write the ASCII values directly into memory at known addresses.
    // This avoids needing string literals for each character.

    // Unshifted map  (0x9000 base)
    uint8_t* base = 0x9000;

    // Row: scancodes 0x00-0x0F
    __asm__("movb $0,  0(%0)\n\t"   // 0x00 - none
        "movb $27, 1(%0)\n\t"   // 0x01 - ESC
        "movb $49, 2(%0)\n\t"   // 0x02 - '1'
        "movb $50, 3(%0)\n\t"   // 0x03 - '2'
        "movb $51, 4(%0)\n\t"   // 0x04 - '3'
        "movb $52, 5(%0)\n\t"   // 0x05 - '4'
        "movb $53, 6(%0)\n\t"   // 0x06 - '5'
        "movb $54, 7(%0)\n\t"   // 0x07 - '6'
        "movb $55, 8(%0)\n\t"   // 0x08 - '7'
        "movb $56, 9(%0)\n\t"   // 0x09 - '8'
        "movb $57, 10(%0)\n\t"  // 0x0A - '9'
        "movb $48, 11(%0)\n\t"  // 0x0B - '0'
        "movb $45, 12(%0)\n\t"  // 0x0C - '-'
        "movb $61, 13(%0)\n\t"  // 0x0D - '='
        "movb $8,  14(%0)\n\t"  // 0x0E - Backspace
        "movb $9,  15(%0)\n\t"  // 0x0F - Tab
        : : "r"(base));

    // Row: scancodes 0x10-0x1F
    __asm__("movb $113, 16(%0)\n\t"  // 0x10 - 'q'
        "movb $119, 17(%0)\n\t"  // 0x11 - 'w'
        "movb $101, 18(%0)\n\t"  // 0x12 - 'e'
        "movb $114, 19(%0)\n\t"  // 0x13 - 'r'
        "movb $116, 20(%0)\n\t"  // 0x14 - 't'
        "movb $121, 21(%0)\n\t"  // 0x15 - 'y'
        "movb $117, 22(%0)\n\t"  // 0x16 - 'u'
        "movb $105, 23(%0)\n\t"  // 0x17 - 'i'
        "movb $111, 24(%0)\n\t"  // 0x18 - 'o'
        "movb $112, 25(%0)\n\t"  // 0x19 - 'p'
        "movb $91,  26(%0)\n\t"  // 0x1A - '['
        "movb $93,  27(%0)\n\t"  // 0x1B - ']'
        "movb $10,  28(%0)\n\t"  // 0x1C - Enter
        "movb $0,   29(%0)\n\t"  // 0x1D - Left Ctrl
        "movb $97,  30(%0)\n\t"  // 0x1E - 'a'
        "movb $115, 31(%0)\n\t"  // 0x1F - 's'
        : : "r"(base));

    // Row: scancodes 0x20-0x2F
    __asm__("movb $100, 32(%0)\n\t"  // 0x20 - 'd'
        "movb $102, 33(%0)\n\t"  // 0x21 - 'f'
        "movb $103, 34(%0)\n\t"  // 0x22 - 'g'
        "movb $104, 35(%0)\n\t"  // 0x23 - 'h'
        "movb $106, 36(%0)\n\t"  // 0x24 - 'j'
        "movb $107, 37(%0)\n\t"  // 0x25 - 'k'
        "movb $108, 38(%0)\n\t"  // 0x26 - 'l'
        "movb $59,  39(%0)\n\t"  // 0x27 - ';'
        "movb $39,  40(%0)\n\t"  // 0x28 - '\''
        "movb $96,  41(%0)\n\t"  // 0x29 - '`'
        "movb $0,   42(%0)\n\t"  // 0x2A - Left Shift
        "movb $92,  43(%0)\n\t"  // 0x2B - '\'
        "movb $122, 44(%0)\n\t"  // 0x2C - 'z'
        "movb $120, 45(%0)\n\t"  // 0x2D - 'x'
        "movb $99,  46(%0)\n\t"  // 0x2E - 'c'
        "movb $118, 47(%0)\n\t"  // 0x2F - 'v'
        : : "r"(base));

    // Row: scancodes 0x30-0x3F
    __asm__("movb $98,  48(%0)\n\t"  // 0x30 - 'b'
        "movb $110, 49(%0)\n\t"  // 0x31 - 'n'
        "movb $109, 50(%0)\n\t"  // 0x32 - 'm'
        "movb $44,  51(%0)\n\t"  // 0x33 - ','
        "movb $46,  52(%0)\n\t"  // 0x34 - '.'
        "movb $47,  53(%0)\n\t"  // 0x35 - '/'
        "movb $0,   54(%0)\n\t"  // 0x36 - Right Shift
        "movb $42,  55(%0)\n\t"  // 0x37 - '*' (numpad)
        "movb $0,   56(%0)\n\t"  // 0x38 - Alt
        "movb $32,  57(%0)\n\t"  // 0x39 - Space
        "movb $0,   58(%0)\n\t"  // 0x3A - CapsLock
        "movb $0,   59(%0)\n\t"  // 0x3B - F1
        "movb $0,   60(%0)\n\t"  // 0x3C - F2
        "movb $0,   61(%0)\n\t"  // 0x3D - F3
        "movb $0,   62(%0)\n\t"  // 0x3E - F4
        "movb $0,   63(%0)\n\t"  // 0x3F - F5
        : : "r"(base));

    // Row: scancodes 0x40-0x4F
    __asm__("movb $0,  64(%0)\n\t"   // 0x40 - F6
        "movb $0,  65(%0)\n\t"   // 0x41 - F7
        "movb $0,  66(%0)\n\t"   // 0x42 - F8
        "movb $0,  67(%0)\n\t"   // 0x43 - F9
        "movb $0,  68(%0)\n\t"   // 0x44 - F10
        "movb $0,  69(%0)\n\t"   // 0x45 - NumLock
        "movb $0,  70(%0)\n\t"   // 0x46 - ScrollLock
        "movb $55, 71(%0)\n\t"   // 0x47 - '7' numpad
        "movb $56, 72(%0)\n\t"   // 0x48 - '8' numpad (up)
        "movb $57, 73(%0)\n\t"   // 0x49 - '9' numpad
        "movb $45, 74(%0)\n\t"   // 0x4A - '-' numpad
        "movb $52, 75(%0)\n\t"   // 0x4B - '4' numpad (left)
        "movb $53, 76(%0)\n\t"   // 0x4C - '5' numpad
        "movb $54, 77(%0)\n\t"   // 0x4D - '6' numpad (right)
        "movb $43, 78(%0)\n\t"   // 0x4E - '+' numpad
        "movb $49, 79(%0)\n\t"   // 0x4F - '1' numpad (end)
        : : "r"(base));

    // Row: scancodes 0x50-0x58
    __asm__("movb $50,  80(%0)\n\t"  // 0x50 - '2' numpad (down)
        "movb $51,  81(%0)\n\t"  // 0x51 - '3' numpad (pgdn)
        "movb $48,  82(%0)\n\t"  // 0x52 - '0' numpad (ins)
        "movb $46,  83(%0)\n\t"  // 0x53 - '.' numpad (del)
        "movb $0,   84(%0)\n\t"
        "movb $0,   85(%0)\n\t"
        "movb $0,   86(%0)\n\t"
        "movb $0,   87(%0)\n\t"  // 0x57 - F11
        "movb $0,   88(%0)\n\t"  // 0x58 - F12
        : : "r"(base));

    // ─── Shifted map (0x9080 base) ─────────────────────────────────────────────
    uint8_t* sbase = 0x9080;

    // Shifted row 0x00-0x0F
    __asm__("movb $0,   0(%0)\n\t"   // none
        "movb $27,  1(%0)\n\t"   // ESC (same)
        "movb $33,  2(%0)\n\t"   // '!'
        "movb $64,  3(%0)\n\t"   // '@'
        "movb $35,  4(%0)\n\t"   // '#'
        "movb $36,  5(%0)\n\t"   // '$'
        "movb $37,  6(%0)\n\t"   // '%%'
        "movb $94,  7(%0)\n\t"   // '^'
        "movb $38,  8(%0)\n\t"   // '&'
        "movb $42,  9(%0)\n\t"   // '*'
        "movb $40, 10(%0)\n\t"   // '('
        "movb $41, 11(%0)\n\t"   // ')'
        "movb $95, 12(%0)\n\t"   // '_'
        "movb $43, 13(%0)\n\t"   // '+'
        "movb $8,  14(%0)\n\t"   // Backspace (same)
        "movb $9,  15(%0)\n\t"   // Tab (same)
        : : "r"(sbase));

    // Shifted row 0x10-0x1F  (uppercase letters)
    __asm__("movb $81,  16(%0)\n\t"  // 'Q'
        "movb $87,  17(%0)\n\t"  // 'W'
        "movb $69,  18(%0)\n\t"  // 'E'
        "movb $82,  19(%0)\n\t"  // 'R'
        "movb $84,  20(%0)\n\t"  // 'T'
        "movb $89,  21(%0)\n\t"  // 'Y'
        "movb $85,  22(%0)\n\t"  // 'U'
        "movb $73,  23(%0)\n\t"  // 'I'
        "movb $79,  24(%0)\n\t"  // 'O'
        "movb $80,  25(%0)\n\t"  // 'P'
        "movb $123, 26(%0)\n\t"  // '{'
        "movb $125, 27(%0)\n\t"  // '}'
        "movb $10,  28(%0)\n\t"  // Enter
        "movb $0,   29(%0)\n\t"  // Ctrl
        "movb $65,  30(%0)\n\t"  // 'A'
        "movb $83,  31(%0)\n\t"  // 'S'
        : : "r"(sbase));

    // Shifted row 0x20-0x2F
    __asm__("movb $68,  32(%0)\n\t"  // 'D'
        "movb $70,  33(%0)\n\t"  // 'F'
        "movb $71,  34(%0)\n\t"  // 'G'
        "movb $72,  35(%0)\n\t"  // 'H'
        "movb $74,  36(%0)\n\t"  // 'J'
        "movb $75,  37(%0)\n\t"  // 'K'
        "movb $76,  38(%0)\n\t"  // 'L'
        "movb $58,  39(%0)\n\t"  // ':'
        "movb $34,  40(%0)\n\t"  // '"'
        "movb $126, 41(%0)\n\t"  // '~'
        "movb $0,   42(%0)\n\t"  // Shift
        "movb $124, 43(%0)\n\t"  // '|'
        "movb $90,  44(%0)\n\t"  // 'Z'
        "movb $88,  45(%0)\n\t"  // 'X'
        "movb $67,  46(%0)\n\t"  // 'C'
        "movb $86,  47(%0)\n\t"  // 'V'
        : : "r"(sbase));

    // Shifted row 0x30-0x39
    __asm__("movb $66,  48(%0)\n\t"  // 'B'
        "movb $78,  49(%0)\n\t"  // 'N'
        "movb $77,  50(%0)\n\t"  // 'M'
        "movb $60,  51(%0)\n\t"  // '<'
        "movb $62,  52(%0)\n\t"  // '>'
        "movb $63,  53(%0)\n\t"  // '?'
        "movb $0,   54(%0)\n\t"  // Shift
        "movb $42,  55(%0)\n\t"  // '*'
        "movb $0,   56(%0)\n\t"  // Alt
        "movb $32,  57(%0)\n\t"  // Space
        : : "r"(sbase));
}

// ─── Saved mouse position for partial redraws ────────────────────────────────
uint32_t _last_mouse_x = 0;
uint32_t _last_mouse_y = 0;

// ─── Read one scancode from the keyboard controller ───────────────────────────
uint8_t _get_scancode() {
    gui_sync_vga();
    while (true) {
        uint32_t status = inb(KEYBOARD_STATUS_PORT);
        if ((status & 1) != 0) {
            if ((status & 0x20) != 0) {
                // Mouse data — use fast partial redraw instead of full re-render
                uint32_t ready = handle_mouse_byte(inb(KEYBOARD_DATA_PORT));
                if (ready == 1) {
                    uint32_t nx = get_mouse_x();
                    uint32_t ny = get_mouse_y();
                    if (gui_vga_is_active() != 0) { gui_sync_vga(); }
                    gui_update_mouse(_last_mouse_x, _last_mouse_y, nx, ny);
                    _last_mouse_x = nx;
                    _last_mouse_y = ny;
                }
            } else {
                // Keyboard data
                return inb(KEYBOARD_DATA_PORT);
            }
        }
    }
    return 0;
}

// ─── Read a full key event ────────────────────────────────────────────────────
// Returns a packed u32:  [mods:8][special:8][ascii:8][0:8]
// Use key_ascii(), key_special(), key_mods() helpers to unpack.
uint32_t read_key() {
    while (true) {
        uint32_t sc = _get_scancode();

        // Extended key prefix — next scancode is an extended key
        if (sc == 0xE0) {
            extended = 1;
            continue;
        }

        // Key release — upper bit set (0x80)
        if ((sc & 0x80) != 0) {
            uint8_t base_sc = sc & 0x7F;
            // Track modifier releases
            if (base_sc == 0x2A || base_sc == 0x36) { shift_down = 0; }
            if (base_sc == 0x1D) { ctrl_down = 0; }
            if (base_sc == 0x38) { alt_down = 0; }
            extended = 0;
            continue;
        }

        // ── Handle modifiers / toggles ─────────────────────────────────────
        if (sc == 0x2A || sc == 0x36) { shift_down = 1; extended = 0; continue; }  // Shift
        if (sc == 0x1D) { ctrl_down = 1; extended = 0; continue; }                  // Ctrl
        if (sc == 0x38) { alt_down  = 1; extended = 0; continue; }                  // Alt
        if (sc == 0x3A) { // CapsLock toggle
            if (caps_lock == 0) { caps_lock = 1; } else { caps_lock = 0; }
            extended = 0;
            continue;
        }

        // ── Extended keys (arrows, Home, End, etc.) ────────────────────────
        uint8_t special = KEY_NONE;
        if (extended != 0) {
            if (sc == 0x48) { special = KEY_UP; }
            if (sc == 0x50) { special = KEY_DOWN; }
            if (sc == 0x4B) { special = KEY_LEFT; }
            if (sc == 0x4D) { special = KEY_RIGHT; }
            if (sc == 0x47) { special = KEY_HOME; }
            if (sc == 0x4F) { special = KEY_END; }
            if (sc == 0x49) { special = KEY_PGUP; }
            if (sc == 0x51) { special = KEY_PGDN; }
            if (sc == 0x52) { special = KEY_INSERT; }
            if (sc == 0x53) { special = KEY_DEL; }
            extended = 0;
            if (special != KEY_NONE) {
                uint8_t mods = (ctrl_down * MOD_CTRL) | (shift_down * MOD_SHIFT) | (alt_down * MOD_ALT);
                uint32_t result = 0;
                __asm__("movzbl %b1, %0\n\t" "shll $8, %0\n\t"
                    : "=r"(result) : "q"(special));
                uint32_t m32 = 0;
                __asm__("movzbl %b1, %0\n\t" : "=r"(m32) : "q"(mods));
                result = result | (m32 << 16);
                return result;
            }
            continue;
        }

        // ── F-keys ─────────────────────────────────────────────────────────
        if (sc == 0x3B) { special = KEY_F1;  }
        if (sc == 0x3C) { special = KEY_F2;  }
        if (sc == 0x3D) { special = KEY_F3;  }
        if (sc == 0x3E) { special = KEY_F4;  }
        if (sc == 0x3F) { special = KEY_F5;  }
        if (sc == 0x40) { special = KEY_F6;  }
        if (sc == 0x41) { special = KEY_F7;  }
        if (sc == 0x42) { special = KEY_F8;  }
        if (sc == 0x43) { special = KEY_F9;  }
        if (sc == 0x44) { special = KEY_F10; }
        if (sc == 0x57) { special = KEY_F11; }
        if (sc == 0x58) { special = KEY_F12; }

        if (special != KEY_NONE) {
            uint8_t mods = (ctrl_down * MOD_CTRL) | (shift_down * MOD_SHIFT) | (alt_down * MOD_ALT);
            uint32_t result = 0;
            __asm__("movzbl %b1, %0\n\t" "shll $8, %0\n\t"
                : "=r"(result) : "q"(special));
            uint32_t m32 = 0;
            __asm__("movzbl %b1, %0\n\t" : "=r"(m32) : "q"(mods));
            result = result | (m32 << 16);
            return result;
        }

        // ── Regular keys — look up in keymap ───────────────────────────────
        uint8_t* base = 0x9000;
        uint8_t* sbase = 0x9080;

        // Bounds check (scancode must be < 128)
        if (sc >= 128) { continue; }

        uint8_t ascii = 0;

        // Determine shifted state: shift XOR caps (caps only flips letters)
        uint32_t shifted = shift_down;

        // Check if this is a letter key (result from unshifted map is a-z, 97-122)
        uint8_t unshifted_ch = 0;
        __asm__("movzbl %b1, %%eax\n\t"
            "add %2, %%eax\n\t"
            "movb (%%eax), %b0\n\t"
            : "=q"(unshifted_ch) : "q"(sc), "r"(base) : "%eax");

        if (unshifted_ch >= 97 && unshifted_ch <= 122) {
            // It's a letter — caps XOR shift decides case
            if (caps_lock != shift_down) {
                shifted = 1;
            } else {
                shifted = 0;
            }
        }

        if (shifted != 0) {
            __asm__("movzbl %b1, %%eax\n\t"
                "add %2, %%eax\n\t"
                "movb (%%eax), %b0\n\t"
                : "=q"(ascii) : "q"(sc), "r"(sbase) : "%eax");
        } else {
            ascii = unshifted_ch;
        }

        if (ascii == 0) { continue; }   // Unmapped key, ignore

        // Apply Ctrl: strip bit 6 (Ctrl+letter = ASCII control code)
        if (ctrl_down != 0 && ascii >= 64 && ascii <= 95) {
            ascii = ascii & 0x1F;
        }
        if (ctrl_down != 0 && ascii >= 97 && ascii <= 122) {
            ascii = (ascii - 32) & 0x1F;
        }

        uint8_t mods = (ctrl_down * MOD_CTRL) | (shift_down * MOD_SHIFT) | (alt_down * MOD_ALT);
        if (caps_lock != 0) { mods = mods | MOD_CAPS; }

        uint32_t result = 0;
        __asm__("movzbl %b1, %0\n\t" : "=r"(result) : "q"(ascii));
        uint32_t m32 = 0;
        __asm__("movzbl %b1, %0\n\t" : "=r"(m32) : "q"(mods));
        result = result | (m32 << 16);
        return result;
    }
    return 0;
}

// ─── Unpack helpers ───────────────────────────────────────────────────────────

uint8_t key_ascii(uint32_t ev) {
    uint8_t r = 0;
    __asm__("movl %1, %%eax\n\t"
        "andl $0xFF, %%eax\n\t"
        "movb %%al, %b0\n\t"
        : "=q"(r) : "r"(ev) : "%eax");
    return r;
}

uint8_t key_special(uint32_t ev) {
    uint8_t r = 0;
    __asm__("movl %1, %%eax\n\t"
        "shrl $8, %%eax\n\t"
        "andl $0xFF, %%eax\n\t"
        "movb %%al, %b0\n\t"
        : "=q"(r) : "r"(ev) : "%eax");
    return r;
}

uint8_t key_mods(uint32_t ev) {
    uint8_t r = 0;
    __asm__("movl %1, %%eax\n\t"
        "shrl $16, %%eax\n\t"
        "andl $0xFF, %%eax\n\t"
        "movb %%al, %b0\n\t"
        : "=q"(r) : "r"(ev) : "%eax");
    return r;
}

uint8_t key_is_ctrl(uint32_t ev) {
    uint32_t mods = key_mods(ev);
    return mods & MOD_CTRL;
}









// ─── edit.nux — Dual-Mode Text Editor (Nano + Vim) ──────────────────────────
//
// F1 = switch to Nano mode (mode-less)
// F2 = switch to Vim mode (modal)
// Default: Nano mode
//
// Nano mode:  Type to insert. Ctrl+S = save. Ctrl+X = exit (prompt save).
//             Arrow keys move cursor.  Backspace deletes.
//
// Vim mode:   Normal mode by default.
//             i = insert before cursor  a = insert after  ESC = back to normal
//             h/j/k/l = left/down/up/right (also arrow keys work everywhere)
//             :w = save   :q = quit   :wq = save+quit   :q! = quit no save
//             x = delete char  dd = delete line (not yet implemented)

// ─── Editor buffer constants ──────────────────────────────────────────────────
uint8_t* EDIT_BUF = 0x00500000;    // 512 KB editor text buffer
uint32_t* EDIT_ROWS = 0x00580000;   // Row start offsets (2048 rows max × 4 bytes)
uint32_t MAX_EDIT_LINES = 2048;
uint32_t MAX_EDIT_COLS = 79;        // 80 columns - 1
uint32_t SCREEN_ROWS = 23;        // Reserve top row for title, bottom for status

// ─── Editor State ─────────────────────────────────────────────────────────────
uint32_t edit_buf_len = 0;
uint32_t edit_cursor = 0;     // Byte offset into EDIT_BUF
uint32_t edit_row = 0;     // Current visible row cursor is on
uint32_t edit_col = 0;     // Current column
uint32_t edit_scroll = 0;     // First visible line index
uint8_t* edit_filename = 0;
uint8_t edit_modified = 0;

// VIM mode flags
uint8_t NANO_MODE = 0;
uint8_t VIM_MODE = 1;
uint8_t edit_mode = 0;       // 0 = nano, 1 = vim
uint8_t vim_insert = 0;      // 1 = vim insert mode active

// Vim command buffer for : commands
uint8_t* vim_cmd_buf = 0x004F0000;
uint32_t vim_cmd_len = 0;
uint8_t vim_in_colon = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────

void _edit_putch(uint8_t* buf, uint32_t offset, uint8_t ch) {
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buf), "r"(offset), "q"(ch));
}

uint8_t _edit_getch(uint8_t* buf, uint32_t offset) {
    uint8_t c = 0;
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
        : "=q"(c) : "r"(buf), "r"(offset) : "%eax");
    return c;
}

uint32_t _edit_strlen(uint8_t* buf) {
    uint32_t i = 0;
    while (true) {
        uint32_t c = _edit_getch(buf, i);
        if (c == 0) { return i; }
        i += 1;
    }
    return 0;
}

// ─── Buffer Insert / Delete ───────────────────────────────────────────────────

// Insert one byte at cursor position
void _buf_insert(uint8_t ch) {
    if (edit_buf_len >= 524288) { return; } // Buffer full
    // Shift bytes from cursor to end right by 1
    uint32_t i = edit_buf_len;
    while (i > edit_cursor) {
        uint32_t prev = i - 1;
        uint32_t c = _edit_getch(EDIT_BUF, prev);
        _edit_putch(EDIT_BUF, i, c);
        i -= 1;
    }
    _edit_putch(EDIT_BUF, edit_cursor, ch);
    edit_buf_len += 1;
    edit_cursor += 1;
    edit_modified = 1;
}

// Delete byte before cursor (backspace)
void _buf_backspace() {
    if (edit_cursor == 0) { return; }
    edit_cursor -= 1;
    uint32_t i = edit_cursor;
    while (i < edit_buf_len - 1) {
        uint32_t next = i + 1;
        uint32_t c = _edit_getch(EDIT_BUF, next);
        _edit_putch(EDIT_BUF, i, c);
        i += 1;
    }
    edit_buf_len -= 1;
    edit_modified = 1;
}

// Delete byte at cursor position (Delete key)
void _buf_delete_at() {
    if (edit_cursor >= edit_buf_len) { return; }
    uint32_t i = edit_cursor;
    while (i < edit_buf_len - 1) {
        uint32_t next = i + 1;
        uint32_t c = _edit_getch(EDIT_BUF, next);
        _edit_putch(EDIT_BUF, i, c);
        i += 1;
    }
    edit_buf_len -= 1;
    edit_modified = 1;
}

// ─── Load file into buffer ────────────────────────────────────────────────────

void _load_file(uint8_t* name) {
    uint32_t data = get_file_data(name);
    uint32_t sz = get_file_size(name);
    if (data == 0) {
        edit_buf_len = 0;
        _edit_putch(EDIT_BUF, 0, 0);
        return;
    }
    // Copy data into edit buffer
    uint32_t i = 0;
    while (i < sz) {
        uint32_t c = _edit_getch(data, i);
        _edit_putch(EDIT_BUF, i, c);
        i += 1;
    }
    _edit_putch(EDIT_BUF, sz, 0);
    edit_buf_len = sz;
}

// ─── Save buffer to RamFS ─────────────────────────────────────────────────────

void _save_file() {
    _edit_putch(EDIT_BUF, edit_buf_len, 0);
    write_file(edit_filename, EDIT_BUF, edit_buf_len);
    edit_modified = 0;
}

// ─── Cursor Navigation ────────────────────────────────────────────────────────

void _cursor_move_left() {
    if (edit_cursor > 0) { edit_cursor -= 1; }
}

void _cursor_move_right() {
    if (edit_cursor < edit_buf_len) { edit_cursor += 1; }
}

void _cursor_move_up() {
    // Find start of current line
    uint32_t pos = edit_cursor;
    if (pos > 0) { pos -= 1; }
    while (pos > 0) {
        uint32_t c = _edit_getch(EDIT_BUF, pos);
        if (c == 10) { break; }
        pos -= 1;
    }
    if (pos == 0) { edit_cursor = 0; return; }
    // Now find start of previous line
    pos -= 1;
    uint32_t line_end = pos;
    while (pos > 0) {
        uint32_t c = _edit_getch(EDIT_BUF, pos);
        if (c == 10) { pos += 1; break; }
        pos -= 1;
    }
    // Move to same column
    uint32_t target_col = edit_col;
    uint32_t new_pos = pos;
    uint32_t col = 0;
    while (col < target_col) {
        uint32_t c = _edit_getch(EDIT_BUF, new_pos);
        if (c == 10 || c == 0 || new_pos > line_end) { break; }
        new_pos += 1;
        col += 1;
    }
    edit_cursor = new_pos;
}

void _cursor_move_down() {
    // Scan forward to next newline
    uint32_t pos = edit_cursor;
    while (pos < edit_buf_len) {
        uint32_t c = _edit_getch(EDIT_BUF, pos);
        if (c == 10) { pos += 1; break; }
        pos += 1;
    }
    if (pos >= edit_buf_len) { return; }
    // Move to same column
    uint32_t target_col = edit_col;
    uint32_t col = 0;
    while (col < target_col && pos < edit_buf_len) {
        uint32_t c = _edit_getch(EDIT_BUF, pos);
        if (c == 10 || c == 0) { break; }
        pos += 1;
        col += 1;
    }
    edit_cursor = pos;
}

// Recompute edit_row and edit_col from edit_cursor
void _update_row_col() {
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t i = 0;
    while (i < edit_cursor) {
        uint32_t c = _edit_getch(EDIT_BUF, i);
        if (c == 10) {
            row += 1;
            col = 0;
        } else {
            col += 1;
        }
        i += 1;
    }
    edit_row = row;
    edit_col = col;
}

// ─── Screen Rendering ─────────────────────────────────────────────────────────

void _render_title_bar(uint8_t* filename, uint8_t* mode_name) {
    set_color(COLOR_WHITE, COLOR_BLUE);
    clear_row(0);
    print_at("  Nux Editor  [", 0, 0);
    print_at(filename, 15, 0);
    print_at("]  Mode: ", 15 + _edit_strlen(filename), 0);
    print_at(mode_name, 24 + _edit_strlen(filename), 0);
    if (edit_modified != 0) {
        print_at(" *", 30 + _edit_strlen(filename), 0);
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _render_status_bar_nano() {
    set_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    clear_row(24);
    print_at("^S Save  ^X Exit  ^C Interrupt  F2 Vim Mode  Arrows Move", 0, 24);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _render_status_bar_vim(uint8_t in_insert) {
    set_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    clear_row(24);
    if (vim_in_colon != 0) {
        print_at(":", 0, 24);
        print_at(vim_cmd_buf, 1, 24);
    } else if (in_insert != 0) {
        set_color(COLOR_WHITE, COLOR_GREEN);
        print_at("-- INSERT --  ESC=Normal  Arrows Move  Backspace Del", 0, 24);
    } else {
        print_at("NORMAL: i=Insert  :w=Save  :q=Quit  :wq=Save+Quit  :q!=Force Quit", 0, 24);
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _render_content() {
    _update_row_col();

    // Adjust scroll
    if (edit_row < edit_scroll) { edit_scroll = edit_row; }
    if (edit_row >= edit_scroll + SCREEN_ROWS) { edit_scroll = edit_row - SCREEN_ROWS + 1; }

    uint32_t screen_row = 1;   // Row 0 is title bar, row 24 is status
    uint32_t buf_row = 0;
    uint32_t i = 0;

    // Find start of scroll row
    while (buf_row < edit_scroll && i < edit_buf_len) {
        uint32_t c = _edit_getch(EDIT_BUF, i);
        if (c == 10) { buf_row += 1; }
        i += 1;
    }

    // Render each visible line
    while (screen_row <= SCREEN_ROWS) {
        clear_row(screen_row);
        uint32_t col = 0;
        while (i <= edit_buf_len && col < 80) {
            uint32_t c = _edit_getch(EDIT_BUF, i);
            if (c == 10 || c == 0 || i == edit_buf_len) { break; }
            print_char_at(c, col, screen_row);
            col += 1;
            i += 1;
        }
        // Skip past newline
        if (i < edit_buf_len) {
            uint32_t c = _edit_getch(EDIT_BUF, i);
            if (c == 10) { i += 1; }
        }
        screen_row += 1;
        buf_row += 1;
    }

    // Position cursor on screen
    uint32_t cursor_screen_row = edit_row - edit_scroll + 1;
    set_cursor(edit_col, cursor_screen_row);
}

// ─── Vim Colon Command Execution ──────────────────────────────────────────────

// Returns 1 if should quit, 0 otherwise
uint8_t _execute_vim_cmd() {
    _edit_putch(vim_cmd_buf, vim_cmd_len, 0);
    uint32_t cmd = vim_cmd_buf;
    if (eq(cmd, "w")) {
        _save_file();
        return 0;
    }
    if (eq(cmd, "q")) {
        if (edit_modified != 0) {
            // Show warning, don't quit
            return 0;
        }
        return 1;
    }
    if (eq(cmd, "wq") || eq(cmd, "x")) {
        _save_file();
        return 1;
    }
    if (eq(cmd, "q!")) {
        return 1;
    }
    return 0;
}

// ─── Public Entry Point ───────────────────────────────────────────────────────

void editor_open(uint8_t* filename) {
    init_keymaps();

    edit_filename = filename;
    edit_cursor = 0;
    edit_row = 0;
    edit_col = 0;
    edit_scroll = 0;
    edit_modified = 0;
    edit_mode = NANO_MODE;
    vim_insert = 0;
    vim_in_colon = 0;
    vim_cmd_len = 0;

    _load_file(filename);

    clear();

    while (true) {
        if (edit_mode == NANO_MODE) {
            _render_title_bar(filename, "NANO");
            _render_status_bar_nano();
        } else {
            _render_title_bar(filename, "VIM");
            _render_status_bar_vim(vim_insert);
        }
        _render_content();

        uint32_t ev = read_key();
        uint32_t ascii = key_ascii(ev);
        uint32_t special = key_special(ev);
        uint32_t mods = key_mods(ev);
        uint32_t is_ctrl = key_is_ctrl(ev);

        // ── Mode switch via F1/F2 ──────────────────────────────────────────
        if (special == KEY_F1) { edit_mode = NANO_MODE; vim_insert = 0; continue; }
        if (special == KEY_F2) { edit_mode = VIM_MODE;  vim_insert = 0; continue; }

        // ── Arrow keys (work in all modes) ────────────────────────────────
        if (special == KEY_UP)    { _cursor_move_up();    continue; }
        if (special == KEY_DOWN)  { _cursor_move_down();  continue; }
        if (special == KEY_LEFT)  { _cursor_move_left();  continue; }
        if (special == KEY_RIGHT) { _cursor_move_right(); continue; }

        if (special == KEY_HOME)  { 
            // Go to start of line
            while (edit_cursor > 0) {
                uint32_t c = _edit_getch(EDIT_BUF, edit_cursor - 1);
                if (c == 10) { break; }
                edit_cursor -= 1;
            }
            continue; 
        }
        if (special == KEY_END)   { 
            // Go to end of line
            while (edit_cursor < edit_buf_len) {
                uint32_t c = _edit_getch(EDIT_BUF, edit_cursor);
                if (c == 10) { break; }
                edit_cursor += 1;
            }
            continue; 
        }

        // ── Nano Mode ──────────────────────────────────────────────────────
        if (edit_mode == NANO_MODE) {
            // Ctrl+X: exit
            if (ascii == 24) { // Ctrl+X
                if (edit_modified != 0) {
                    set_color(COLOR_BLACK, COLOR_YELLOW);
                    clear_row(24);
                    print_at("Save before exit? (S=yes N=no)", 0, 24);
                    set_color(COLOR_WHITE, COLOR_BLACK);
                    uint32_t ans = read_key();
                    uint32_t ans_ch = key_ascii(ans);
                    if (ans_ch == 115 || ans_ch == 83) { _save_file(); }
                }
                clear();
                return;
            }
            // Ctrl+S: save
            if (ascii == 19) { _save_file(); continue; }
            // Ctrl+C: clear / interrupt (stay in editor)
            if (ascii == 3) { continue; }
            // Backspace
            if (ascii == 8) { _buf_backspace(); continue; }
            // Delete key
            if (special == KEY_DEL) { _buf_delete_at(); continue; }
            // Enter
            if (ascii == 10 || ascii == 13) { _buf_insert(10); continue; }
            // Tab
            if (ascii == 9) { _buf_insert(32); _buf_insert(32); _buf_insert(32); _buf_insert(32); continue; }
            // Printable
            if (ascii >= 32 && ascii < 127) { _buf_insert(ascii); continue; }
            continue;
        }

        // ── Vim Mode — Normal ──────────────────────────────────────────────
        if (edit_mode == VIM_MODE && vim_insert == 0) {

            // Colon command mode
            if (vim_in_colon != 0) {
                if (ascii == 10 || ascii == 13) {
                    vim_in_colon = 0;
                    uint32_t quit = _execute_vim_cmd();
                    vim_cmd_len = 0;
                    if (quit != 0) { clear(); return; }
                } else if (ascii == 8) {
                    if (vim_cmd_len > 0) { vim_cmd_len -= 1; }
                } else if (ascii == 27) { // ESC
                    vim_in_colon = 0;
                    vim_cmd_len = 0;
                } else if (ascii >= 32 && ascii < 127) {
                    _edit_putch(vim_cmd_buf, vim_cmd_len, ascii);
                    vim_cmd_len += 1;
                }
                continue;
            }

            // Normal mode commands
            if (ascii == 58) { vim_in_colon = 1; vim_cmd_len = 0; continue; } // ':'
            if (ascii == 105) { vim_insert = 1; continue; } // 'i'
            if (ascii == 97)  { _cursor_move_right(); vim_insert = 1; continue; } // 'a'
            if (ascii == 65)  { // 'A' — go to end of line then insert
                while (edit_cursor < edit_buf_len) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor);
                    if (c == 10) { break; }
                    edit_cursor += 1;
                }
                vim_insert = 1;
                continue;
            }
            if (ascii == 73) { // 'I' — go to start of line then insert
                while (edit_cursor > 0) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor - 1);
                    if (c == 10) { break; }
                    edit_cursor -= 1;
                }
                vim_insert = 1;
                continue;
            }
            if (ascii == 111) { // 'o' — open line below
                while (edit_cursor < edit_buf_len) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor);
                    if (c == 10) { edit_cursor += 1; break; }
                    edit_cursor += 1;
                }
                _buf_insert(10);
                edit_cursor -= 1;
                vim_insert = 1;
                continue;
            }
            // h j k l movement
            if (ascii == 104) { _cursor_move_left();  continue; } // 'h'
            if (ascii == 106) { _cursor_move_down();  continue; } // 'j'
            if (ascii == 107) { _cursor_move_up();    continue; } // 'k'
            if (ascii == 108) { _cursor_move_right(); continue; } // 'l'
            // x = delete char at cursor
            if (ascii == 120) { _buf_delete_at(); continue; }
            // w = jump forward a word
            if (ascii == 119) {
                while (edit_cursor < edit_buf_len) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor);
                    if (c == 32 || c == 10) { edit_cursor += 1; break; }
                    edit_cursor += 1;
                }
                continue;
            }
            // b = jump back a word
            if (ascii == 98) {
                if (edit_cursor > 0) { edit_cursor -= 1; }
                while (edit_cursor > 0) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor - 1);
                    if (c == 32 || c == 10) { break; }
                    edit_cursor -= 1;
                }
                continue;
            }
            // 0 = start of line
            if (ascii == 48) {
                while (edit_cursor > 0) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor - 1);
                    if (c == 10) { break; }
                    edit_cursor -= 1;
                }
                continue;
            }
            // $ = end of line
            if (ascii == 36) {
                while (edit_cursor < edit_buf_len) {
                    uint32_t c = _edit_getch(EDIT_BUF, edit_cursor);
                    if (c == 10) { break; }
                    edit_cursor += 1;
                }
                continue;
            }
            continue;
        }

        // ── Vim Mode — Insert ──────────────────────────────────────────────
        if (edit_mode == VIM_MODE && vim_insert != 0) {
            // ESC exits insert mode
            if (ascii == 27) { vim_insert = 0; if (edit_cursor > 0) { edit_cursor -= 1; } continue; }
            if (ascii == 8)  { _buf_backspace(); continue; }
            if (special == KEY_DEL) { _buf_delete_at(); continue; }
            if (ascii == 10 || ascii == 13) { _buf_insert(10); continue; }
            if (ascii == 9) { _buf_insert(32); _buf_insert(32); _buf_insert(32); _buf_insert(32); continue; }
            if (ascii >= 32 && ascii < 127) { _buf_insert(ascii); continue; }
        }
    }
}









// ─── fm.nux — Full-Screen File Manager ───────────────────────────────────────
//
// Layout (80×25 VGA):
//   Row 0:    Title bar (blue)
//   Rows 1-21: File listing (left column) + Dir listing (right column)
//   Row 22:   Separator
//   Row 23:   Status bar / prompt area
//   Row 24:   Shortcuts bar
//
// Keys:
//   Up/Down     Navigate cursor
//   Enter       Open file (in editor) or open directory (not yet)
//   n           New file (prompt for name)
//   m           New directory (prompt for name)
//   d / Del     Delete selected file or directory
//   r           Rename selected file or directory
//   Tab         Switch focus between files / dirs panels
//   q / Esc     Quit file manager

uint32_t FM_LIST_ROWS = 21;    // Rows 1-21
uint32_t FM_LEFT_COL = 0;
uint32_t FM_RIGHT_COL = 40;

// State
uint32_t fm_file_cursor = 0;
uint32_t fm_dir_cursor = 0;
uint8_t fm_focus = 0;   // 0 = files panel, 1 = dirs panel
uint32_t fm_file_scroll = 0;
uint32_t fm_dir_scroll = 0;

// Input prompt buffer (for new file/dir name prompts)
uint8_t* PROMPT_BUF = 0x004E0000;
uint32_t prompt_len = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────

uint32_t _fm_str_len(uint8_t* s) {
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
            : "=q"(c) : "r"(s), "r"(i) : "%eax");
        if (c == 0) { return i; }
        i += 1;
    }
    return 0;
}

void _fm_null_term(uint8_t* buf, uint32_t len) {
    __asm__("movb $0, (%0,%1,1)\n\t" : : "r"(buf), "r"(len));
}

// ─── Rendering ────────────────────────────────────────────────────────────────

void _fm_render_title() {
    set_color(COLOR_WHITE, COLOR_BLUE);
    clear_row(0);
    print_at("  Nux File Manager   [Files]                 [Directories]", 0, 0);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _fm_render_separator() {
    set_color(COLOR_DARK_GREY, COLOR_BLACK);
    clear_row(22);
    uint32_t col = 0;
    while (col < 80) {
        print_char_at(196, col, 22);  // '─' box char
        col += 1;
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _fm_render_shortcuts() {
    set_color(COLOR_BLACK, COLOR_LIGHT_GREY);
    clear_row(24);
    print_at("n=NewFile m=NewDir d=Del r=Rename Enter=Open/Edit Tab=Panel q=Quit", 0, 24);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _fm_render_status(uint8_t* msg) {
    set_color(COLOR_YELLOW, COLOR_BLACK);
    clear_row(23);
    print_at(msg, 0, 23);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _fm_render_files() {
    uint32_t row = 0;
    while (row < FM_LIST_ROWS) {
        uint32_t file_idx = row + fm_file_scroll;
        uint32_t screen_row = row + 1;

        // Determine if this row is the cursor row (and if we're focused on files)
        uint8_t is_selected = 0;
        if (fm_focus == 0 && file_idx == fm_file_cursor) { is_selected = 1; }

        uint32_t fname = get_file_name_at(file_idx);

        if (fname == 0) {
            // Empty row
            set_color(COLOR_WHITE, COLOR_BLACK);
            clear_cols(FM_LEFT_COL, 38, screen_row);
        } else {
            if (is_selected != 0) {
                if (fm_focus == 0) {
                    set_color(COLOR_BLACK, COLOR_CYAN);
                } else {
                    set_color(COLOR_WHITE, COLOR_DARK_GREY);
                }
            } else {
                set_color(COLOR_WHITE, COLOR_BLACK);
            }
            clear_cols(FM_LEFT_COL, 38, screen_row);
            print_char_at(32, FM_LEFT_COL, screen_row);
            print_at(fname, FM_LEFT_COL + 2, screen_row);

            // Print file size on right of left column
            uint32_t sz = get_file_size(fname);
            print_u32_at(sz, 34, screen_row);
            print_char_at(32, 39, screen_row);
        }
        row += 1;
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void _fm_render_dirs() {
    uint32_t row = 0;
    while (row < FM_LIST_ROWS) {
        uint32_t dir_idx = row + fm_dir_scroll;
        uint32_t screen_row = row + 1;

        uint8_t is_selected = 0;
        if (fm_focus == 1 && dir_idx == fm_dir_cursor) { is_selected = 1; }

        uint32_t dname = get_dir_name_at(dir_idx);

        if (dname == 0) {
            set_color(COLOR_WHITE, COLOR_BLACK);
            clear_cols(FM_RIGHT_COL, 38, screen_row);
        } else {
            if (is_selected != 0) {
                if (fm_focus == 1) {
                    set_color(COLOR_BLACK, COLOR_YELLOW);
                } else {
                    set_color(COLOR_WHITE, COLOR_DARK_GREY);
                }
            } else {
                set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            }
            clear_cols(FM_RIGHT_COL, 38, screen_row);
            print_char_at(47, FM_RIGHT_COL, screen_row);  // '/'
            print_at(dname, FM_RIGHT_COL + 1, screen_row);
        }
        row += 1;
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

// ─── Input Prompt ─────────────────────────────────────────────────────────────

// Reads a line into PROMPT_BUF from the keyboard.
// Draws into row 23 (status row).  Returns 1 if confirmed, 0 if ESC'd.
uint8_t _prompt(uint8_t* msg) {
    prompt_len = 0;
    _fm_null_term(PROMPT_BUF, 0);

    while (true) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        clear_row(23);
        print_at(msg, 0, 23);
        uint32_t msg_len = _fm_str_len(msg);
        print_at(PROMPT_BUF, msg_len, 23);
        set_color(COLOR_WHITE, COLOR_BLACK);

        uint32_t ev = read_key();
        uint32_t ascii = key_ascii(ev);
        uint32_t special = key_special(ev);

        if (ascii == 27 || special == KEY_ESC) { return 0; }
        if (ascii == 10 || ascii == 13) {
            _fm_null_term(PROMPT_BUF, prompt_len);
            return 1;
        }
        if (ascii == 8 && prompt_len > 0) {
            prompt_len -= 1;
            _fm_null_term(PROMPT_BUF, prompt_len);
            continue;
        }
        if (ascii >= 32 && ascii < 127 && prompt_len < 14) {
            __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(PROMPT_BUF), "r"(prompt_len), "q"(ascii));
            prompt_len += 1;
            _fm_null_term(PROMPT_BUF, prompt_len);
        }
    }
    return 0;
}

// ─── Public Entry Point ───────────────────────────────────────────────────────

void filemanager_open() {
    init_keymaps();

    fm_file_cursor = 0;
    fm_dir_cursor  = 0;
    fm_focus       = 0;
    fm_file_scroll = 0;
    fm_dir_scroll  = 0;

    clear();

    while (true) {
        _fm_render_title();
        _fm_render_files();

        // Draw vertical divider (column 39)
        uint32_t r = 1;
        while (r <= FM_LIST_ROWS) {
            set_color(COLOR_DARK_GREY, COLOR_BLACK);
            print_char_at(179, 39, r);  // '│'
            r += 1;
        }
        set_color(COLOR_WHITE, COLOR_BLACK);

        _fm_render_dirs();
        _fm_render_separator();
        _fm_render_shortcuts();

        // Status: show selected file/dir info
        if (fm_focus == 0) {
            uint32_t fname = get_file_name_at(fm_file_cursor);
            if (fname != 0) {
                set_color(COLOR_CYAN, COLOR_BLACK);
                clear_row(23);
                print_at("File: ", 0, 23);
                print_at(fname, 6, 23);
                print_at("  Size: ", 6 + _fm_str_len(fname), 23);
                print_u32_at(get_file_size(fname), 14 + _fm_str_len(fname), 23);
                set_color(COLOR_WHITE, COLOR_BLACK);
            }
        } else {
            uint32_t dname = get_dir_name_at(fm_dir_cursor);
            if (dname != 0) {
                set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                clear_row(23);
                print_at("Dir: /", 0, 23);
                print_at(dname, 6, 23);
                set_color(COLOR_WHITE, COLOR_BLACK);
            }
        }

        uint32_t ev = read_key();
        uint32_t ascii = key_ascii(ev);
        uint32_t special = key_special(ev);

        // ── Quit ──────────────────────────────────────────────────────────
        if (ascii == 113 || ascii == 27) { // 'q' or ESC
            clear();
            return;
        }

        // ── Tab: switch panel ──────────────────────────────────────────────
        if (ascii == 9) {
            if (fm_focus == 0) { fm_focus = 1; } else { fm_focus = 0; }
            continue;
        }

        // ── Arrow navigation ───────────────────────────────────────────────
        if (special == KEY_UP) {
            if (fm_focus == 0 && fm_file_cursor > 0) {
                fm_file_cursor -= 1;
                if (fm_file_cursor < fm_file_scroll) { fm_file_scroll -= 1; }
            }
            if (fm_focus == 1 && fm_dir_cursor > 0) {
                fm_dir_cursor -= 1;
                if (fm_dir_cursor < fm_dir_scroll) { fm_dir_scroll -= 1; }
            }
            continue;
        }
        if (special == KEY_DOWN) {
            if (fm_focus == 0) {
                uint32_t next = fm_file_cursor + 1;
                if (get_file_name_at(next) != 0 || next < file_count) {
                    fm_file_cursor = next;
                    if (fm_file_cursor >= fm_file_scroll + FM_LIST_ROWS) { fm_file_scroll += 1; }
                }
            }
            if (fm_focus == 1) {
                uint32_t next = fm_dir_cursor + 1;
                if (get_dir_name_at(next) != 0 || next < dir_count) {
                    fm_dir_cursor = next;
                    if (fm_dir_cursor >= fm_dir_scroll + FM_LIST_ROWS) { fm_dir_scroll += 1; }
                }
            }
            continue;
        }

        // ── Enter: open in editor (files panel only) ───────────────────────
        if (ascii == 10 || ascii == 13) {
            if (fm_focus == 0) {
                uint32_t fname = get_file_name_at(fm_file_cursor);
                if (fname != 0) {
                    editor_open(fname);
                    clear();
                }
            }
            continue;
        }

        // ── n: new file ────────────────────────────────────────────────────
        if (ascii == 110) { // 'n'
            uint32_t ok = _prompt("New file name: ");
            if (ok != 0 && prompt_len > 0) {
                uint8_t* empty = "";
                uint32_t result = create_file(PROMPT_BUF, empty, 0);
                if (result == 0) {
                    _fm_render_status("Error: file already exists or too many files.");
                } else {
                    // Open the new empty file in editor
                    editor_open(PROMPT_BUF);
                    clear();
                }
            }
            continue;
        }

        // ── m: new directory ───────────────────────────────────────────────
        if (ascii == 109) { // 'm'
            uint32_t ok = _prompt("New directory name: ");
            if (ok != 0 && prompt_len > 0) {
                uint32_t result = create_dir(PROMPT_BUF);
                if (result == 0) {
                    _fm_render_status("Error: directory already exists or too many dirs.");
                }
            }
            continue;
        }

        // ── d / Delete: delete ─────────────────────────────────────────────
        if (ascii == 100 || special == KEY_DEL) { // 'd' or Del
            if (fm_focus == 0) {
                uint32_t fname = get_file_name_at(fm_file_cursor);
                if (fname != 0) {
                    set_color(COLOR_WHITE, COLOR_RED);
                    clear_row(23);
                    print_at("Delete '", 0, 23);
                    print_at(fname, 8, 23);
                    print_at("'? (Y/N)", 8 + _fm_str_len(fname), 23);
                    set_color(COLOR_WHITE, COLOR_BLACK);
                    uint32_t conf = read_key();
                    uint32_t conf_ch = key_ascii(conf);
                    if (conf_ch == 121 || conf_ch == 89) { // 'y' or 'Y'
                        delete_file(fname);
                        if (fm_file_cursor > 0) { fm_file_cursor -= 1; }
                    }
                }
            } else {
                uint32_t dname = get_dir_name_at(fm_dir_cursor);
                if (dname != 0) {
                    set_color(COLOR_WHITE, COLOR_RED);
                    clear_row(23);
                    print_at("Delete dir '", 0, 23);
                    print_at(dname, 12, 23);
                    print_at("'? (Y/N)", 12 + _fm_str_len(dname), 23);
                    set_color(COLOR_WHITE, COLOR_BLACK);
                    uint32_t conf = read_key();
                    uint32_t conf_ch = key_ascii(conf);
                    if (conf_ch == 121 || conf_ch == 89) {
                        delete_dir(dname);
                        if (fm_dir_cursor > 0) { fm_dir_cursor -= 1; }
                    }
                }
            }
            continue;
        }

        // ── r: rename ──────────────────────────────────────────────────────
        if (ascii == 114) { // 'r'
            if (fm_focus == 0) {
                uint32_t fname = get_file_name_at(fm_file_cursor);
                if (fname != 0) {
                    uint32_t ok = _prompt("Rename file to: ");
                    if (ok != 0 && prompt_len > 0) {
                        uint32_t data = get_file_data(fname);
                        uint32_t size = get_file_size(fname);
                        uint32_t res = create_file(PROMPT_BUF, data, size);
                        if (res != 0) {
                            delete_file(fname);
                        } else {
                            _fm_render_status("Error: cannot rename file.");
                        }
                    }
                }
            } else {
                uint32_t dname = get_dir_name_at(fm_dir_cursor);
                if (dname != 0) {
                    uint32_t ok = _prompt("Rename dir to: ");
                    if (ok != 0 && prompt_len > 0) {
                        uint32_t res = create_dir(PROMPT_BUF);
                        if (res != 0) {
                            delete_dir(dname);
                        } else {
                            _fm_render_status("Error: cannot rename directory.");
                        }
                    }
                }
            }
            continue;
        }
    }
}


















// ─── shell.nux — Navi OS Interactive Shell ────────────────────────────────────
//
// Input loop: reads one key at a time via read_key().
// Supports:
//   Backspace     Delete character before cursor
//   Ctrl+C        Clear current input buffer (interrupt)
//   Ctrl+L        Clear screen (same as `clear`)
//   Enter         Execute command
//   All printable keys from full QWERTY driver
//
// Commands:
//   help          List commands
//   clear / cls   Clear screen
//   echo <msg>    Print a message
//   uname[-a]     OS info
//   whoami        Current user
//   pwd           Print working directory (always / for now)
//   ls            List files and directories
//   cat <file>    Print file contents
//   touch <file>  Create an empty file
//   rm <file>     Delete a file
//   mkdir <dir>   Create a directory
//   rmdir <dir>   Delete a directory
//   edit <file>   Open file in Nux Editor
//   fm            Open Nux File Manager
//   df            Show disk (ramfs) free/used space
//   head <file>   Print first 10 lines of file
//   tail <file>   Print last 10 lines of file
//   wc <file>     Print newline, word, and byte counts
//   sleep <ms>    Halt CPU for rough duration
//   ps            List processes
//   kill <pid>    Kill a process
//   bg <cmd>      Start process in background
//   fg <pid>      Bring process to foreground
//   jobs          List background jobs
//   free          Show memory usage
//   cc <file>     Compile and run a C/C++ file (LLVM stub)
//   desktop       Switch to Graphical Desktop Mode
//   view <file>   Open Photo Viewer
//   threads       List all threads
//   vfs-ls        List VFS root directory
//   vfs-tree      Show full VFS directory tree
//   uptime        Show ticks since boot
//   cd <dir>      Change directory
//   halt          Halt the CPU

uint8_t* CMD_BUF = 0x00200000;    // 64 KB command buffer
uint32_t CMD_MAX = 511;

uint8_t* DIR_BUF = 0x00210000;    // Buffer for current directory string
uint8_t* current_dir = "/";


void _shell_putch(uint8_t* buf, uint32_t idx, uint8_t ch) {
    __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(buf), "r"(idx), "q"(ch));
}

uint8_t _shell_getch(uint8_t* buf, uint32_t idx) {
    uint8_t c = 0;
    __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t"
        : "=q"(c) : "r"(buf), "r"(idx) : "%eax");
    return c;
}

// ─── Read input line ──────────────────────────────────────────────────────────
// Fills CMD_BUF with a null-terminated string.
// Returns the length of the entered string.
uint32_t _read_line() {
    uint32_t idx = 0;

    while (true) {
        uint32_t ev = read_key();
        uint32_t ascii = key_ascii(ev);
        uint32_t special = key_special(ev);

        // ── Enter ──────────────────────────────────────────────────────────
        if (ascii == 10 || ascii == 13) {
            print_char(10);
            _shell_putch(CMD_BUF, idx, 0);  // null-terminate
            return idx;
        }

        // ── Backspace ──────────────────────────────────────────────────────
        if (ascii == 8 || special == KEY_BACKSPACE) {
            if (idx > 0) {
                idx -= 1;
                _shell_putch(CMD_BUF, idx, 0);
                print_char(8);   // move cursor back
                print_char(32);  // overwrite with space
                print_char(8);   // move cursor back again
            }
            continue;
        }

        // ── Ctrl+C: interrupt / clear line ────────────────────────────────
        if (ascii == 3) {  // Ctrl+C = ASCII ETX
            print("^C\n");
            print("root@navi:~> ");
            idx = 0;
            continue;
        }

        // ── Ctrl+L: clear screen ───────────────────────────────────────────
        if (ascii == 12) {  // Ctrl+L = ASCII FF
            clear();
            print("root@navi:~> ");
            // Reprint what was typed so far
            uint32_t j = 0;
            while (j < idx) {
                uint32_t c = _shell_getch(CMD_BUF, j);
                print_char(c);
                j += 1;
            }
            continue;
        }

        // ── Printable characters ───────────────────────────────────────────
        if (ascii >= 32 && ascii < 127) {
            if (idx < CMD_MAX) {
                _shell_putch(CMD_BUF, idx, ascii);
                idx += 1;
                print_char(ascii);
            }
            continue;
        }

        // Other special keys (arrows, F-keys) are ignored in the shell
    }
    return 0;
}

// ─── Command Handlers ─────────────────────────────────────────────────────────

void _cmd_ls() {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("Files:\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    uint32_t i = 0;
    while (i < file_count) {
        uint32_t fname = get_file_name_at(i);
        if (fname != 0) {
            print("  ");
            print(fname);
            print("  (");
            print_u32(get_file_size(fname));
            print(" bytes)\n");
        }
        i += 1;
    }
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    print("Directories:\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    i = 0;
    while (i < dir_count) {
        uint32_t dname = get_dir_name_at(i);
        if (dname != 0) {
            print("  /");
            print(dname);
            print("/\n");
        }
        i += 1;
    }
}

void _cmd_cat(uint8_t* fname) {
    uint32_t data = get_file_data(fname);
    if (data == 0) {
        print("cat: ");
        print(fname);
        print(": No such file\n");
        return;
    }
    print(data);
    print_char(10);
}

void _cmd_touch(uint8_t* fname) {
    uint8_t* empty = "";
    uint32_t ok = create_file(fname, empty, 0);
    if (ok == 0) {
        print("touch: ");
        print(fname);
        print(": file already exists\n");
    }
}

void _cmd_rm(uint8_t* fname) {
    uint32_t ok = delete_file(fname);
    if (ok == 0) {
        print("rm: ");
        print(fname);
        print(": No such file\n");
    }
}

void _cmd_mkdir(uint8_t* dname) {
    uint32_t ok = create_dir(dname);
    if (ok == 0) {
        print("mkdir: ");
        print(dname);
        print(": cannot create directory\n");
    }
}

void _cmd_rmdir(uint8_t* dname) {
    uint32_t ok = delete_dir(dname);
    if (ok == 0) {
        print("rmdir: ");
        print(dname);
        print(": No such directory\n");
    }
}

uint8_t* _find_first_space(uint8_t* s) {
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(s), "r"(i) : "%eax");
        if (c == 0) { return 0; }
        if (c == 32) { return s + i; }
        i += 1;
    }
    return 0;
}

void _cmd_cp(uint8_t* args) {
    uint32_t space = _find_first_space(args);
    if (space == 0) {
        print("cp: missing destination file\n");
        return;
    }
    __asm__("movb $0, (%0)" : : "r"(space));
    uint32_t src = args;
    uint32_t dst = space + 1;
    
    uint32_t data = get_file_data(src);
    if (data == 0) {
        print("cp: "); print(src); print(": No such file\n");
        return;
    }
    uint32_t size = get_file_size(src);
    uint32_t ok = create_file(dst, data, size);
    if (ok == 0) {
        print("cp: cannot create destination file\n");
    }
}

void _cmd_mv(uint8_t* args) {
    uint32_t space = _find_first_space(args);
    if (space == 0) {
        print("mv: missing destination file\n");
        return;
    }
    __asm__("movb $0, (%0)" : : "r"(space));
    uint32_t src = args;
    uint32_t dst = space + 1;
    
    uint32_t data = get_file_data(src);
    if (data == 0) {
        print("mv: "); print(src); print(": No such file\n");
        return;
    }
    uint32_t size = get_file_size(src);
    uint32_t ok = create_file(dst, data, size);
    if (ok == 0) {
        print("mv: cannot move to destination\n");
        return;
    }
    delete_file(src);
}

void _cmd_df() {
    uint32_t used = 0;
    uint32_t i = 0;
    while (i < file_count) {
        uint32_t fname = get_file_name_at(i);
        if (fname != 0) {
            used += get_file_size(fname);
        }
        i += 1;
    }
    print("Filesystem      Size  Used Avail Use% Mounted on\n");
    print("ramfs           64MB  ");
    print_u32(used);
    print("   ---   ---  /\n");
}

void _cmd_head(uint8_t* fname) {
    uint32_t data = get_file_data(fname);
    if (data == 0) { print("head: No such file\n"); return; }
    
    uint32_t i = 0;
    uint32_t lines = 0;
    while (lines < 10) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(data), "r"(i) : "%eax");
        if (c == 0) { break; }
        print_char(c);
        if (c == 10) { lines += 1; }
        i += 1;
    }
    if (lines < 10 && i > 0) { print_char(10); }
}

void _cmd_wc(uint8_t* fname) {
    uint32_t data = get_file_data(fname);
    if (data == 0) { print("wc: No such file\n"); return; }
    
    uint32_t size = get_file_size(fname);
    uint32_t lines = 0;
    uint32_t words = 0;
    uint8_t in_word = 0;
    
    uint32_t i = 0;
    while (i < size) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(data), "r"(i) : "%eax");
        if (c == 10) { lines += 1; }
        if (c == 32 || c == 10 || c == 9 || c == 13) {
            in_word = 0;
        } else if (in_word == 0) {
            in_word = 1;
            words += 1;
        }
        i += 1;
    }
    
    print("  "); print_u32(lines);
    print("  "); print_u32(words);
    print("  "); print_u32(size);
    print(" "); print(fname); print("\n");
}

uint32_t _parse_int(uint8_t* s) {
    uint32_t res = 0;
    uint32_t i = 0;
    while (true) {
        uint8_t c = 0;
        __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(s), "r"(i) : "%eax");
        if (c < 48 || c > 57) { break; }
        res = (res * 10) + (c - 48);
        i += 1;
    }
    return res;
}

void _cmd_kill(uint8_t* args) {
    uint32_t pid = _parse_int(args);
    if (pid == 0) {
        print("kill: invalid pid\n");
        return;
    }
    if (pid == 1) {
        print("kill: cannot kill init/shell process\n");
        return;
    }
    uint32_t ok = kill_process(pid);
    if (ok == 1) {
        print("Process killed.\n");
    } else {
        print("kill: no such process\n");
    }
}

void _cmd_bg(uint8_t* args) {
    uint32_t pid = create_process(args);
    if (pid != 0) {
        print("["); print_u32(pid); print("] ");
        print(args);
        print("\n");
    } else {
        print("bg: cannot create process (max limit reached)\n");
    }
}

void _cmd_fg(uint8_t* args) {
    uint32_t pid = _parse_int(args);
    if (pid == 0) { print("fg: invalid pid\n"); return; }
    // Dummy implementation for now, just pretend to resume it
    print("Resumed process ");
    print_u32(pid);
    print(" in foreground.\n");
}

void _cmd_cd(uint8_t* args) {
    // Since ramfs is flat right now, just pretend to change directory
    if (eq(args, "/") || eq(args, "..")) {
        current_dir = "/";
    } else {
        // Copy to DIR_BUF so it isn't overwritten by CMD_BUF
        uint32_t i = 0;
        while (true) {
            uint8_t c = 0;
            __asm__("movzbl (%1,%2,1), %%eax\n\t" "movb %%al, %b0\n\t" : "=q"(c) : "r"(args), "r"(i) : "%eax");
            __asm__("movb %b2, (%0,%1,1)\n\t" : : "r"(DIR_BUF), "r"(i), "q"(c));
            if (c == 0) { break; }
            i += 1;
        }
        current_dir = DIR_BUF;
    }
}

void _cmd_cc(uint8_t* args) {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("LLVM JIT Compiler & Executor (Stub)\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    print("Analyzing source: ");
    print(args);
    print("\n");
    print("Error: LLVM backend and standard library not yet ported to Nux!\n");
    print("To implement fully in Nux, we must bootstrap a native Nux IR compiler.\n");
}

void _cmd_help() {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("Navi OS — Available Commands\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    print("  help           This help message\n");
    print("  clear / cls    Clear screen  (also Ctrl+L)\n");
    print("  echo <msg>     Print message\n");
    print("  uname          OS version\n");
    print("  whoami         Current user\n");
    print("  pwd            Print working directory\n");
    print("  ls             List files and directories\n");
    print("  cat <file>     Print file contents\n");
    print("  touch <file>   Create empty file\n");
    print("  cp <src> <dst> Copy a file\n");
    print("  mv <src> <dst> Move/rename a file\n");
    print("  rm <file>      Delete file\n");
    print("  mkdir <dir>    Create directory\n");
    print("  rmdir <dir>    Delete directory\n");
    print("  df             Disk usage\n");
    print("  head <file>    Show first 10 lines\n");
    print("  wc <file>      Count words/lines\n");
    print("  edit <file>    Open Nux Editor (F1=Nano F2=Vim)\n");
    print("  fm             Open Nux File Manager\n");
    print("  ps             List running processes\n");
    print("  kill <pid>     Kill a process by PID\n");
    print("  bg <cmd>       Run command as background job\n");
    print("  fg <pid>       Bring job to foreground\n");
    print("  jobs           List background jobs\n");
    print("  free           Show memory usage\n");
    print("  cc <file>      Compile/run C/C++ file (LLVM stub)\n");
    print("  view <file>    Open image in Photo Viewer\n");
    print("  cd <dir>       Change working directory\n");
    print("  sleep <ms>     Sleep (dummy loop)\n");
    print("  halt           Halt the system\n");
    set_color(COLOR_DARK_GREY, COLOR_BLACK);
    print("  Ctrl+C: interrupt   Ctrl+L: clear screen   Backspace: delete\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
}

// ─── Shell Main Loop ──────────────────────────────────────────────────────────

void start_shell() {
    init_keymaps();
    init_ramfs();
    populate_readme();

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("Welcome to Navi OS\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    print("Type 'help' for a list of commands.\n\n");

    while (true) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print("root@navi:");
        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        print(current_dir);
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print("> ");
        set_color(COLOR_WHITE, COLOR_BLACK);

        uint32_t len = _read_line();

        if (len == 0) { continue; }

        // ── Dispatch ───────────────────────────────────────────────────────
        if (eq(CMD_BUF, "exit")) {
            return;
        } else if (eq(CMD_BUF, "help")) {
            _cmd_help();
        } else if (eq(CMD_BUF, "clear") || eq(CMD_BUF, "cls")) {
            clear();
        } else if (eq(CMD_BUF, "pwd")) {
            print(current_dir);
            print_char(10);
        } else if (eq(CMD_BUF, "whoami")) {
            print("root\n");
        } else if (eq(CMD_BUF, "uname") || eq(CMD_BUF, "uname -a")) {
            print("Navi OS 1.0.0 i686 nux (Pure Nux Kernel)\n");
        } else if (eq(CMD_BUF, "ls")) {
            _cmd_ls();
        } else if (eq(CMD_BUF, "fm")) {
            filemanager_open();
        } else if (eq(CMD_BUF, "halt") || eq(CMD_BUF, "shutdown")) {
            set_color(COLOR_YELLOW, COLOR_BLACK);
            print("System halted. Goodbye.\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
            halt_forever();
        } else if (starts_with(CMD_BUF, "echo ")) {
            print(CMD_BUF + 5);
            print_char(10);
        } else if (starts_with(CMD_BUF, "cat ")) {
            _cmd_cat(CMD_BUF + 4);
        } else if (starts_with(CMD_BUF, "touch ")) {
            _cmd_touch(CMD_BUF + 6);
        } else if (starts_with(CMD_BUF, "cp ")) {
            _cmd_cp(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "mv ")) {
            _cmd_mv(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "rm ")) {
            _cmd_rm(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "mkdir ")) {
            _cmd_mkdir(CMD_BUF + 6);
        } else if (starts_with(CMD_BUF, "rmdir ")) {
            _cmd_rmdir(CMD_BUF + 6);
        } else if (starts_with(CMD_BUF, "head ")) {
            _cmd_head(CMD_BUF + 5);
        } else if (starts_with(CMD_BUF, "wc ")) {
            _cmd_wc(CMD_BUF + 3);
        } else if (eq(CMD_BUF, "df")) {
            _cmd_df();
        } else if (eq(CMD_BUF, "free")) {
            print("Total RAM:   32 MB (Mocked)\n");
            print("Heap Used:   ");
            print_u32(get_allocated_memory());
            print(" bytes\n");
        } else if (starts_with(CMD_BUF, "cc ")) {
            _cmd_cc(CMD_BUF + 3);
        } else if (eq(CMD_BUF, "ps") || eq(CMD_BUF, "jobs")) {
            print_ps();
        } else if (starts_with(CMD_BUF, "kill ")) {
            _cmd_kill(CMD_BUF + 5);
        } else if (starts_with(CMD_BUF, "bg ")) {
            _cmd_bg(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "fg ")) {
            _cmd_fg(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "cd ")) {
            _cmd_cd(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "view ")) {
            open_image(CMD_BUF + 5);
        } else if (eq(CMD_BUF, "threads")) {
            print_threads();
        } else if (eq(CMD_BUF, "vfs-ls")) {
            vfs_ls(0);
        } else if (eq(CMD_BUF, "vfs-tree")) {
            vfs_tree();
        } else if (eq(CMD_BUF, "uptime")) {
            print("Ticks since boot: ");
            print_u32(get_ticks());
            print(" (100 Hz = 10ms/tick)\n");
        } else if (starts_with(CMD_BUF, "sleep ")) {
            // Dummy delay loop
            uint32_t k = 0;
            while (k < 10000000) { k += 1; }
        } else if (starts_with(CMD_BUF, "edit ")) {
            editor_open(CMD_BUF + 5);
            clear();
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            print(CMD_BUF);
            set_color(COLOR_WHITE, COLOR_BLACK);
            print(": command not found\n");
        }
    }
}













// --- desktop.nux -- Graphical Desktop Event Loop --------------------------------
//
// Mouse:
//   Left click   - activate UI elements, close/dock buttons
//   Right click  - context menu stub
//   Middle click - reserved
//   Scroll dir 1 - scroll up
//   Scroll dir 2 - scroll down
//
// Keyboard shortcuts:
//   ESC / Q      - exit desktop, return to text shell
//   R / F5       - force full desktop refresh
//   F1           - show keyboard shortcut help
//   Space/Enter  - refresh desktop

uint8_t desktop_active = 0;

// Dock icon hit regions (must match gui_draw_dock offsets)
uint32_t DOCK_ICON_TERM_X = 8;
uint32_t DOCK_ICON_FILE_X = 72;
uint32_t DOCK_ICON_SET_X = 136;
uint32_t DOCK_ICON_W = 48;
uint32_t DOCK_ICON_H = 48;

void init_desktop() {
    desktop_active = 1;
}

// Compatibility shim used by older shell code
void draw_window(uint8_t* title, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    gui_draw_window(x, y, w, h, title);
}

// --- Rectangle hit test -------------------------------------------------------
uint8_t _in_rect(uint32_t mx, uint32_t my, uint32_t rx, uint32_t ry, uint32_t rw, uint32_t rh) {
    if (mx < rx) { return 0; }
    if (my < ry) { return 0; }
    uint32_t rx2 = 0;
    __asm__("movl %1, %0\n\taddl %2, %0\n\t" : "=r"(rx2) : "r"(rx), "r"(rw));
    uint32_t ry2 = 0;
    __asm__("movl %1, %0\n\taddl %2, %0\n\t" : "=r"(ry2) : "r"(ry), "r"(rh));
    if (mx >= rx2) { return 0; }
    if (my >= ry2) { return 0; }
    return 1;
}

// --- Left click handler -------------------------------------------------------
void _handle_left_click(uint32_t mx, uint32_t my) {
    uint32_t scr_h = fb_get_height();
    uint32_t scr_w = fb_get_width();

    // Dock Y position
    uint32_t dock_y = 0;
    __asm__("movl %1, %0\n\tsubl $48, %0\n\t" : "=r"(dock_y) : "r"(scr_h));

    // Dock: Terminal icon -> start shell
    if (_in_rect(mx, my, DOCK_ICON_TERM_X, dock_y, DOCK_ICON_W, DOCK_ICON_H) == 1) {
        gui_draw_window(40, 40, 656, 444, "Terminal (VGA Emulation)");
        gui_vga_start(40, 40);
        clear();
        start_shell();
        gui_vga_stop();
        gui_render_desktop();
        return;
    }
    // Dock: File manager
    if (_in_rect(mx, my, DOCK_ICON_FILE_X, dock_y, DOCK_ICON_W, DOCK_ICON_H) == 1) {
        gui_draw_window(80, 80, 656, 444, "File Manager (VGA Emulation)");
        gui_vga_start(80, 80);
        clear();
        filemanager_open();
        gui_vga_stop();
        gui_render_desktop();
        return;
    }
    // Dock: Settings (placeholder)
    if (_in_rect(mx, my, DOCK_ICON_SET_X, dock_y, DOCK_ICON_W, DOCK_ICON_H) == 1) {
        gui_draw_window(120, 120, 656, 444, "Settings");
        gui_vga_start(120, 120);
        clear();
        set_color(COLOR_YELLOW, COLOR_BLACK);
        print("\n\n    System Settings\n\n    (Not yet implemented)\n\n    Press ESC to close.");
        set_color(COLOR_WHITE, COLOR_BLACK);
        while (true) {
            uint32_t ev = read_key();
            uint32_t ch = key_ascii(ev);
            if (ch == 27 || key_special(ev) == KEY_ESC) { break; }
        }
        gui_vga_stop();
        gui_render_desktop();
        return;
    }

    // Window close button: window starts at x=40, y=40, w=scr_w-80
    // Close icon is at (win_x + win_w - 20, win_y + 4), 16x16
    uint32_t win_w = 0;
    __asm__("movl %1, %0\n\tsubl $80, %0\n\t" : "=r"(win_w) : "r"(scr_w));
    // close_x = 40 + win_w - 20 = win_w + 20
    uint32_t close_x = 0;
    __asm__("movl %1, %0\n\taddl $20, %0\n\t" : "=r"(close_x) : "r"(win_w));
    // title bar at y=40, buttons at y=44
    if (_in_rect(mx, my, close_x, 44, 16, 16) == 1) {
        desktop_active = 0;
        return;
    }
}

// --- Right click handler (context menu stub) ----------------------------------
void _handle_right_click(uint32_t mx, uint32_t my) {
    // Draw a minimal context menu at click location
    uint32_t menu_w = 140;
    uint32_t menu_h = 88;

    // Clamp so menu stays on screen
    uint32_t scr_w = fb_get_width();
    uint32_t scr_h = fb_get_height();
    uint32_t mx2 = mx;
    uint32_t my2 = my;
    uint32_t right_edge = 0;
    __asm__("movl %1, %0\n\taddl %2, %0\n\t" : "=r"(right_edge) : "r"(mx), "r"(menu_w));
    if (right_edge >= scr_w) {
        __asm__("movl %1, %0\n\tsubl %2, %0\n\t" : "=r"(mx2) : "r"(scr_w), "r"(menu_w));
    }
    uint32_t bot_edge = 0;
    __asm__("movl %1, %0\n\taddl %2, %0\n\t" : "=r"(bot_edge) : "r"(my), "r"(menu_h));
    if (bot_edge >= scr_h) {
        __asm__("movl %1, %0\n\tsubl %2, %0\n\t" : "=r"(my2) : "r"(scr_h), "r"(menu_h));
    }

    // Draw directly to VRAM (menu is temporary, not in backbuffer)
    fb_fill_rect(mx2, my2, menu_w, menu_h, 0x313244);
    fb_draw_rect(mx2, my2, menu_w, menu_h, 0x89B4FA);

    uint32_t tx = 0;
    __asm__("movl %1, %0\n\taddl $8, %0\n\t" : "=r"(tx) : "r"(mx2));
    uint32_t ty = 0;
    __asm__("movl %1, %0\n\taddl $8, %0\n\t" : "=r"(ty) : "r"(my2));

    font_draw_string("Refresh",       tx, ty,       0xCDD6F4, 0x313244);
    ty += 18;
    font_draw_string("Open Terminal", tx, ty,       0xCDD6F4, 0x313244);
    ty += 18;
    font_draw_string("Properties",    tx, ty,       0x6C7086, 0x313244);
    ty += 18;
    fb_draw_hline(mx2, ty, menu_w, 0x6C7086);
    ty += 8;
    font_draw_string("Close Menu",    tx, ty,       0xF38BA8, 0x313244);

    fb_swap_buffers();

    // Wait for next click to dismiss the context menu
    uint8_t dismissed = 0;
    while (dismissed == 0) {
        uint32_t st = inb(0x64);
        if ((st & 1) != 0) {
            if ((st & 0x20) != 0) {
                // Drain mouse byte — any click dismisses
                uint32_t mb = inb(0x60);
                dismissed = 1;
            } else {
                // Any key dismisses too
                inb(0x60);
                dismissed = 1;
            }
        }
    }

    // Re-render desktop to erase the menu
    gui_render_desktop();
}

// --- Show help overlay --------------------------------------------------------
void _show_help_overlay() {
    uint32_t scr_w = fb_get_width();
    uint32_t scr_h = fb_get_height();

    // Center a 320x220 panel
    uint32_t ox = 0;
    uint32_t oy = 0;
    __asm__("movl %1, %0\n\tsubl $320, %0\n\tshrl $1, %0\n\t" : "=r"(ox) : "r"(scr_w));
    __asm__("movl %1, %0\n\tsubl $220, %0\n\tshrl $1, %0\n\t" : "=r"(oy) : "r"(scr_h));

    fb_fill_rect(ox, oy, 320, 220, 0x181825);
    fb_draw_rect(ox, oy, 320, 220, 0x89B4FA);
    gui_draw_window(ox, oy, 320, 220, "Keyboard & Mouse Shortcuts");

    uint32_t tx = 0;
    __asm__("movl %1, %0\n\taddl $10, %0\n\t" : "=r"(tx) : "r"(ox));
    uint32_t ty = 0;
    __asm__("movl %1, %0\n\taddl $34, %0\n\t" : "=r"(ty) : "r"(oy));

    font_draw_string("-- Keyboard --",    tx, ty, 0x89B4FA, 0x181825);
    ty += 16;
    font_draw_string("ESC / Q    Exit desktop", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("R / F5     Force refresh", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("F1         This help", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("Space/Enter  Refresh", tx, ty, 0xCDD6F4, 0x181825);
    ty += 20;
    font_draw_string("-- Mouse --",       tx, ty, 0x89B4FA, 0x181825);
    ty += 16;
    font_draw_string("Left Click   Activate", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("Right Click  Context menu", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("Scroll Up    Scroll content up", tx, ty, 0xCDD6F4, 0x181825);
    ty += 14;
    font_draw_string("Scroll Down  Scroll content down", tx, ty, 0xCDD6F4, 0x181825);
    ty += 20;
    font_draw_string("Press any key to dismiss", tx, ty, 0x6C7086, 0x181825);

    fb_swap_buffers();

    // Wait for any keypress to dismiss
    uint8_t done = 0;
    while (done == 0) {
        uint32_t st = inb(0x64);
        if ((st & 1) != 0) {
            if ((st & 0x20) == 0) {
                uint32_t sc = inb(0x60);
                if ((sc & 0x80) == 0) { done = 1; }  // key-down event
            } else {
                inb(0x60);  // drain mouse byte
            }
        }
    }

    // Re-render after dismissing
    gui_render_desktop();
}

// --- Desktop Main Event Loop --------------------------------------------------
void start_desktop_mode() {
    if (fb_is_active() == 0) {
        print("[Desktop] No framebuffer detected.\n");
        print("  Boot with: set gfxmode=1024x768x32 in grub.cfg\n");
        return;
    }

    init_desktop();
    gui_render_desktop();

    uint32_t last_mx = get_mouse_x();
    uint32_t last_my = get_mouse_y();
    uint8_t prev_btn = 0;
    uint8_t ext_key = 0;  // extended scancode prefix flag (0xE0)

    while (desktop_active != 0) {
        uint32_t status = inb(0x64);
        if ((status & 1) == 0) {
            // Nothing ready — short idle to reduce busy-wait heat
            uint32_t idle = 200;
            while (idle > 0) { idle -= 1; }
            continue;
        }

        if ((status & 0x20) != 0) {
            // ---- Mouse byte -------------------------------------------------
            uint32_t ready = handle_mouse_byte(inb(0x60));
            if (ready == 1) {
                uint32_t nx = get_mouse_x();
                uint32_t ny = get_mouse_y();

                // Fast partial cursor redraw only if position changed
                if (nx != last_mx || ny != last_my) {
                    gui_update_mouse(last_mx, last_my, nx, ny);
                    last_mx = nx;
                    last_my = ny;
                }

                uint32_t btn = get_mouse_buttons();

                // Left click: rising edge detection
                uint8_t left_now = btn & 1;
                uint8_t left_was = prev_btn & 1;
                if (left_now != 0 && left_was == 0) {
                    _handle_left_click(nx, ny);
                }

                // Right click: rising edge detection
                uint8_t right_now = (btn >> 1) & 1;
                uint8_t right_was = (prev_btn >> 1) & 1;
                if (right_now != 0 && right_was == 0) {
                    _handle_right_click(nx, ny);
                }

                // Scroll wheel (0=none, 1=up, 2=down)
                uint32_t scroll = get_mouse_scroll_dir();
                if (scroll != 0) {
                    // Placeholder: future terminal scrollback implementation
                }

                prev_btn = btn;
            }

        } else {
            // ---- Keyboard scancode ------------------------------------------
            uint32_t sc = inb(0x60);

            // Extended prefix (0xE0): set flag, consume next byte
            if (sc == 0xE0) { ext_key = 1; continue; }

            // Key release (bit7): clear ext flag, ignore
            if ((sc & 0x80) != 0) { ext_key = 0; continue; }

            if (ext_key != 0) {
                ext_key = 0;
                // Extended key codes (arrows, Win keys) — ignored in desktop
                continue;
            }

            ext_key = 0;

            // ESC (0x01) or Q (0x10): exit desktop
            if (sc == 0x01 || sc == 0x10) {
                desktop_active = 0;
                continue;
            }

            // R (0x13) or F5 (0x3F): force full refresh
            if (sc == 0x13 || sc == 0x3F) {
                gui_render_desktop();
                last_mx = get_mouse_x();
                last_my = get_mouse_y();
                continue;
            }

            // Space (0x39) or Enter (0x1C): refresh
            if (sc == 0x1C || sc == 0x39) {
                gui_render_desktop();
                last_mx = get_mouse_x();
                last_my = get_mouse_y();
                continue;
            }

            // F1 (0x3B): help overlay
            if (sc == 0x3B) {
                _show_help_overlay();
                last_mx = get_mouse_x();
                last_my = get_mouse_y();
                continue;
            }
        }
    }

    clear();
    print("Returned to Navi OS shell.\n");
    print("Type 'desktop' to re-enter GUI mode.\n");
}






















// kmain is called from _start in boot.nux with two arguments pushed onto the stack:
//   arg0 (mbi_ptr):   physical address of the Multiboot Info struct (from EBX)
//   arg1 (mbi_magic): should be 0x2BADB002 (from EAX, pushed second so it's arg1)

void kmain(uint32_t mbi_magic, uint32_t mbi_ptr) {
    // 1. Memory and process management
    clear();
    init_memory();
    init_processes();

    // 2. Thread management
    init_threads();

    // 3. Hardware interrupt subsystem (PIC + PIT timer, no sti yet)
    init_interrupts();
    init_syscalls();

    // 4. Virtual File System
    init_vfs();
    vfs_populate_tree();

    // 5. Framebuffer / GUI
    // Save the Multiboot info pointer GRUB passed in EBX.
    // fb_read_mbi() will parse it for the VBE framebuffer address/pitch/size.
    fb_save_mbi(mbi_ptr);
    uint32_t fb_ok = fb_read_mbi();
    if (fb_ok == 1) {
        init_gui();
        init_mouse();
        gui_render_desktop();
        // Sync keyboard module's cursor tracking to the actual starting mouse pos
        _last_mouse_x = get_mouse_x();
        _last_mouse_y = get_mouse_y();
        // GUI event loop would go here in a full windowed OS.
        // For now we fall through to the text shell.
    }
    // 6. Hand off to desktop mode (GUI)
    start_desktop_mode();

    // Should never return
    halt_forever();
}


