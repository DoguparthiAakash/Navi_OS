import os
from PIL import Image

# 8x16 VGA font base64-encoded (or just use blocky for now, let's actually just use a simple hex representation of standard IBM VGA 8x16 font)
# Actually, I can use Pillow's default font to render into an 8x16 bitmap.
from PIL import ImageDraw, ImageFont

def generate_font():
    font_data = bytearray(256 * 16)
    # Using default font which might not be 8x16 perfectly, but we will center it.
    fnt = ImageFont.load_default()
    for ch in range(256):
        img = Image.new('1', (8, 16), color=0)
        d = ImageDraw.Draw(img)
        char = chr(ch) if ch >= 32 and ch < 127 else '?'
        if ch == 32: char = ' '
        # Pillow default font is around 6x11, center it
        d.text((1, 2), char, font=fnt, fill=1)
        
        # Convert to 16 bytes
        for y in range(16):
            b = 0
            for x in range(8):
                if img.getpixel((x, y)):
                    b |= (1 << (7 - x))
            font_data[ch * 16 + y] = b
    return font_data

def image_to_argb(path, expected_w=None, expected_h=None):
    if not os.path.exists(path):
        return None
    img = Image.open(path).convert("RGBA")
    if expected_w and expected_h and (img.width != expected_w or img.height != expected_h):
        img = img.resize((expected_w, expected_h))
    
    data = []
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = img.getpixel((x, y))
            # Format: 0xAARRGGBB
            val = (a << 24) | (r << 16) | (g << 8) | b
            data.append(val)
    return img.width, img.height, data

def main():
    out = open("assets.s", "w")
    out.write(".section .rodata\n")

    # 1. Generate Font
    print("Generating Font...")
    font_data = generate_font()
    out.write(".global asset_font\nasset_font:\n")
    for i in range(0, len(font_data), 16):
        b = font_data[i:i+16]
        hex_str = ", ".join([f"0x{x:02x}" for x in b])
        out.write(f".byte {hex_str}\n")

    # 2. Generate Cursor (XCursor-Pro-Dark/left_ptr.png)
    print("Generating Cursor...")
    cursor_path = "bitmaps/XCursor-Pro-Dark/left_ptr.png"
    if os.path.exists(cursor_path):
        # Resize to 24x24 for a good size
        w, h, cursor_data = image_to_argb(cursor_path, 24, 24)
        out.write(f".global asset_cursor\nasset_cursor:\n")
        out.write(f".long {w}, {h}\n") # prefix with width and height
        for val in cursor_data:
            out.write(f".long {val}\n")
    else:
        print(f"Warning: {cursor_path} not found")

    # 3. Generate Icons
    icons = {
        "asset_icon_close": "icons/close.bmp",
        "asset_icon_minimize": "icons/minimize.bmp",
        "asset_icon_maximize": "icons/fullscreen.bmp", # assuming fullscreen is maximize
        "asset_icon_terminal": "icons/terminal.bmp",
        "asset_icon_file": "icons/fileexplorer.bmp",
        "asset_icon_game": "icons/game.bmp",
        "asset_icon_settings": "icons/settings.bmp"
    }

    for sym, path in icons.items():
        print(f"Generating {sym} from {path}...")
        if os.path.exists(path):
            # Icons usually 32x32 or 24x24. Let's force 24x24 for window controls, maybe larger for desktop
            size = 16 if "close" in sym or "minimize" in sym or "maximize" in sym else 48
            w, h, data = image_to_argb(path, size, size)
            out.write(f".global {sym}\n{sym}:\n")
            out.write(f".long {w}, {h}\n")
            for val in data:
                out.write(f".long {val}\n")
        else:
            print(f"Warning: {path} not found")

    out.close()
    print("assets.s generated.")

if __name__ == '__main__':
    main()
