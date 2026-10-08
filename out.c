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

