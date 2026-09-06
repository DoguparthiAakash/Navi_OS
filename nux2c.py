import sys, re, os

def transpile_file(input_file, output_c):
    with open(input_file, 'r') as f:
        content = f.read()

    # Strip @[no_std] and @[entry]
    content = re.sub(r'@\[.*?\]', '', content)
    
    # Comments: # to //
    content = re.sub(r'#(.*)', r'//\1', content)

    # Imports
    content = re.sub(r'import\s+([a-zA-Z0-9_]+);', '', content)

    # Struct setup
    content = re.sub(r'class\s+([a-zA-Z0-9_]+)\s*\{', r'struct \1 {\n', content)
    
    # Assembly
    # Convert asm("...\n...") to __asm__("...\n\t" "...")
    def fix_asm(m):
        s = m.group(1).replace('\n', '\\n\\t" "')
        return f'__asm__("{s}"'
    content = re.sub(r'asm\(\"(.*?)\"', fix_asm, content, flags=re.DOTALL)

    lines = content.split('\n')
    out_lines = []
    in_class = None
    
    for line in lines:
        # Function calls like shell::start_shell() -> start_shell()
        line = re.sub(r'([a-zA-Z0-9_]+)::([a-zA-Z0-9_]+)', r'\2', line)
        
        m = re.match(r'^struct\s+([a-zA-Z0-9_]+)\s*\{', line)
        if m:
            in_class = m.group(1)
            out_lines.append(f"typedef struct {in_class} {in_class};")
            out_lines.append(line)
            continue
            
        if in_class and line.strip() == "}":
            out_lines.append(f"}};")
            in_class = None
            continue
            
        if in_class and re.search(r'func\s+', line):
            # Move func outside class is hard, we just change it to normal function.
            # E.g. func init(name: *u8) -> void init(struct Name* this, uint8_t* name)
            # Actually C doesn't allow functions in structs. So we comment them out for now.
            # In fs.nux, they have func init(...), we can't easily parse that.
            # Let's just comment it out.
            out_lines.append("// " + line)
            continue

        # Variables: var name: type = val; -> type name = val;
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u8\s*=', r'uint8_t* \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u16\s*=', r'uint16_t* \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u32\s*=', r'uint32_t* \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*?fs::File\s*=', r'File* \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*File\s*=', r'File* \1 =', line)
        
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u8\s*=', r'uint8_t \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u16\s*=', r'uint16_t \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u32\s*=', r'uint32_t \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*bool\s*=', r'bool \1 =', line)
        
        # var name: type;
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u8', r'uint8_t* \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u16', r'uint16_t* \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*u32', r'uint32_t* \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*?fs::File', r'File* \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*\*File', r'File* \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*fs::Directory', r'Directory \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*Directory', r'Directory \1', line)
        
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u8', r'uint8_t \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u16', r'uint16_t \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*u32', r'uint32_t \1', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*:\s*bool', r'bool \1', line)
        
        # Untyped vars
        line = re.sub(r'var\s+root\s*=\s*get_root\(\);', r'Directory root = get_root();', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*=', r'uint32_t \1 =', line)
        line = re.sub(r'var\s+([a-zA-Z0-9_]+)\s*;', r'uint32_t \1;', line)
        
        # Functions: func name(args) -> type
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*bool', r'bool \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*u8', r'uint8_t \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*u16', r'uint16_t \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*u32', r'uint32_t \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*\*u8', r'uint8_t* \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)\s*->\s*Directory', r'Directory \1(\2)', line)
        line = re.sub(r'func\s+([a-zA-Z0-9_]+)\s*\((.*?)\)', r'void \1(\2)', line)
        
        # Function arguments translation
        if re.search(r'void\s+[a-zA-Z0-9_]+\s*\(|bool\s+[a-zA-Z0-9_]+\s*\(|uint\d+_t\s+[a-zA-Z0-9_]+\s*\(', line):
            args_m = re.search(r'\((.*?)\)', line)
            if args_m:
                args_str = args_m.group(1)
                new_args = []
                for arg in args_str.split(','):
                    arg = arg.strip()
                    if not arg: continue
                    parts = arg.split(':')
                    if len(parts) == 2:
                        name = parts[0].strip()
                        typ = parts[1].strip()
                        if typ == 'u8': t = 'uint8_t'
                        elif typ == '*u8': t = 'uint8_t*'
                        elif typ == 'u16': t = 'uint16_t'
                        elif typ == '*u16': t = 'uint16_t*'
                        elif typ == 'u32': t = 'uint32_t'
                        elif typ == '*u32': t = 'uint32_t*'
                        elif typ == 'bool': t = 'bool'
                        elif typ == '*File': t = 'File*'
                        else: t = f"{typ}"
                        new_args.append(f"{t} {name}")
                line = line.replace(f"({args_str})", f"({', '.join(new_args)})")
        


        # root.files[i] -> root.files[i]
        
        # Special case fixes for root.files + ... pointer math in C
        line = line.replace('root.files + (i * 12)', '(uint8_t*)root.files + (i * 12)')
        
        # Special case fix for init()
        if 'root_dir.init(' in line:
            line = 'root_dir.name = "/"; root_dir.files = root_files; root_dir.file_count = root_file_count;'
        
        out_lines.append(line)
        
    full_c = "\n".join(out_lines)
    
    # We will just ignore struct methods for now.
    
    with open(output_c, 'w') as f:
        f.write("#include <stdint.h>\n#include <stdbool.h>\n")
        f.write("static inline uint8_t __inb(uint16_t port) { uint8_t ret; __asm__ volatile ( \"inb %1, %0\" : \"=a\"(ret) : \"Nd\"(port) ); return ret; }\n")
        f.write("static inline void __outb(uint16_t port, uint8_t val) { __asm__ volatile ( \"outb %0, %1\" : : \"a\"(val), \"Nd\"(port) ); }\n")
        # Write forward declarations for structs
        if "struct File" in full_c:
            f.write("typedef struct File File;\n")
        if "struct Directory" in full_c:
            f.write("typedef struct Directory Directory;\n")
        f.write(full_c)
        
    print(f"Transpiled {input_file} -> {output_c}")

if __name__ == '__main__':
    transpile_file(sys.argv[1], sys.argv[2])
