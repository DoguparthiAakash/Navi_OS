#include <stdint.h>
#include <stdbool.h>
typedef struct File File;
typedef struct Directory Directory;


// fs.nux - Virtual File System structures

typedef struct File File;
struct File {

    uint8_t* name;
    uint8_t* data;
    uint32_t size;
    
//     func init(name: *u8, data: *u8, size: u32) {
        this.name = name;
        this.data = data;
        this.size = size;
};
}

typedef struct Directory Directory;
struct Directory {

    uint8_t* name;
    File* files; // Pointer to array of files for now
    uint32_t file_count;
    
//     func init(name: *u8, files: *File, count: u32) {
        this.name = name;
        this.files = files;
        this.file_count = count;
};
}
