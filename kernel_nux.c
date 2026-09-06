#include <stdint.h>
#include <stdbool.h>
static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
typedef struct File File;
typedef struct Directory Directory;


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

// ─── Read one scancode from the keyboard controller ───────────────────────────
uint8_t _get_scancode() {
    while (true) {
        uint32_t status = inb(KEYBOARD_STATUS_PORT);
        if ((status & 1) != 0) {
            break;
        }
    }
    return inb(KEYBOARD_DATA_PORT);
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

    // Pre-populate a readme.txt
    uint8_t* readme_content = "Welcome to Navi OS!\nPowered by the Nux language.\nType 'help' in the shell for commands.\n";
    create_file("readme.txt", readme_content, 87);
}

// ─── File Operations ──────────────────────────────────────────────────────────

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
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl %3, (%%eax)\n\t"
        : : "r"(FILE_DATA_PTRS), "r"(ptr_offset), "r"(heap_ptr) : "%eax");

    // Store size
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl %3, (%%eax)\n\t"
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
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl %3, (%%eax)\n\t"
        : : "r"(FILE_DATA_PTRS), "r"(ptr_offset), "r"(heap_ptr) : "%eax");
    __asm__("movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl %3, (%%eax)\n\t"
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

void open(uint8_t* filename) {
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
//   r           Rename (not yet — future)
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
    print_at("n=NewFile  m=NewDir  d=Delete  Enter=Open/Edit  Tab=Panel  q=Quit", 0, 24);
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

void open() {
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
                    open(fname);
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
                    open(PROMPT_BUF);
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
//   halt          Halt the CPU

uint8_t* CMD_BUF = 0x00200000;    // 64 KB command buffer
uint32_t CMD_MAX = 511;

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
            print("root@navi:~// ");
            idx = 0;
            continue;
        }

        // ── Ctrl+L: clear screen ───────────────────────────────────────────
        if (ascii == 12) {  // Ctrl+L = ASCII FF
            clear();
            print("root@navi:~// ");
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
    print("  rm <file>      Delete file\n");
    print("  mkdir <dir>    Create directory\n");
    print("  rmdir <dir>    Delete directory\n");
    print("  edit <file>    Open Nux Editor (F1=Nano F2=Vim)\n");
    print("  fm             Open Nux File Manager\n");
    print("  halt           Halt the system\n");
    set_color(COLOR_DARK_GREY, COLOR_BLACK);
    print("  Ctrl+C: interrupt   Ctrl+L: clear screen   Backspace: delete\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
}

// ─── Shell Main Loop ──────────────────────────────────────────────────────────

void start_shell() {
    init_keymaps();
    init_ramfs();

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    print("Welcome to Navi OS\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    print("Type 'help' for a list of commands.\n\n");

    while (true) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print("root@navi:~// ");
        set_color(COLOR_WHITE, COLOR_BLACK);

        uint32_t len = _read_line();

        if (len == 0) { continue; }

        // ── Dispatch ───────────────────────────────────────────────────────
        if (eq(CMD_BUF, "help")) {
            _cmd_help();
        } else if (eq(CMD_BUF, "clear") || eq(CMD_BUF, "cls")) {
            clear();
        } else if (eq(CMD_BUF, "pwd")) {
            print("/\n");
        } else if (eq(CMD_BUF, "whoami")) {
            print("root\n");
        } else if (eq(CMD_BUF, "uname") || eq(CMD_BUF, "uname -a")) {
            print("Navi OS 1.0.0 i686 nux (Pure Nux Kernel)\n");
        } else if (eq(CMD_BUF, "ls")) {
            _cmd_ls();
        } else if (eq(CMD_BUF, "fm")) {
            open();
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
        } else if (starts_with(CMD_BUF, "rm ")) {
            _cmd_rm(CMD_BUF + 3);
        } else if (starts_with(CMD_BUF, "mkdir ")) {
            _cmd_mkdir(CMD_BUF + 6);
        } else if (starts_with(CMD_BUF, "rmdir ")) {
            _cmd_rmdir(CMD_BUF + 6);
        } else if (starts_with(CMD_BUF, "edit ")) {
            open(CMD_BUF + 5);
            clear();
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            print(CMD_BUF);
            set_color(COLOR_WHITE, COLOR_BLACK);
            print(": command not found\n");
        }
    }
}









// The entry point called by boot.s

void kmain() {
    // Initialize hardware drivers
    clear();
    
    // Hand off execution to the interactive shell
    start_shell();
    
    // We should never return here, but just in case:
    halt_forever();
}

