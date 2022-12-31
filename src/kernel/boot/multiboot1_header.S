#define MAGIC 0x1badb002
#define FLAG_ALIGN    1
#define FLAG_MEM      2
#define FLAG_GFX        4
#define FLAGS           FLAG_ALIGN | FLAG_MEM | FLAG_GFX
#define CHECKSUM - (MAGIC + FLAGS)
#define MODE_TYPE 0 /* request plain framebuffer */
#define WIDTH 1024  /* requested width */
#define HEIGHT 768  /* requested height */
#define DEPTH 32    /* requested bits per pixel BPP */
#define HEADER_ADDR 0
#define LOAD_ADDR 0
#define LOAD_END_ADDR 0
#define BSS_END_ADDR 0
#define ENTRY_ADDR 0

.section .multiboot1
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
    .long HEADER_ADDR
    .long LOAD_ADDR
    .long LOAD_END_ADDR
    .long BSS_END_ADDR
    .long ENTRY_ADDR
    .long MODE_TYPE
    .long WIDTH
    .long HEIGHT
    .long DEPTH
    /* enough space for the returned header */
    .space 4 * 13
