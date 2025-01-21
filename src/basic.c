#include <stdbool.h>
#include <stdint.h>
#include <memory.h>

// Useful macros
#define ArrayLen(x) (sizeof(x) / sizeof(*x))

#define Min(x, y) (((x) < (y)) ? (x) : (y))
#define Max(x, y) (((x) > (y)) ? (x) : (y))
#define Sign(x)   ((x > 0) - (x < 0))
#define Abs(x)    ((x < 0) ? -(x) : (x))
#define Clamp(x, min, max) (Max(Min(x, max), min))

#define Kilobytes(x) ((x) << 10)
#define Megabytes(x) ((x) << 20)
#define Gigabytes(x) ((x) << 30)

#define IsEnabled(x) ((x) != 0)

#define MemoryCopy(ptr, ptr2, len) memcpy(ptr, ptr2, len)
#define MemorySet(ptr, value, len) memset(ptr, value, len)
#define MemoryZero(ptr, len) MemorySet(ptr, 0, len)

// Limits
#define  S8_MIN  (S8)0x80
#define S16_MIN (S16)0x8000
#define S32_MIN (S32)0x80000000
#define S64_MIN (S64)0x8000000000000000LL

#define  S8_MAX  (S8)0x7f
#define S16_MAX (S16)0x7fff
#define S32_MAX (S32)0x7fffffff
#define S64_MAX (S64)0x7fffffffffffffffLL

#define  U8_MAX  (U8)0xff
#define U16_MAX (U16)0xffff
#define U32_MAX (U32)0xffffffff
#define U64_MAX (U64)0xffffffffffffffffULL

// Types
typedef unsigned int uint;

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef U8 Byte;

typedef float  F32;
typedef double F64;

typedef struct Arena Arena;
struct Arena {
    Byte *buffer;
    S64 buffer_len;
    S64 offset;
};

// Globals
Arena global_scratch_arena;

// Useful functions
int is_end_of_line(char c) {
    return (c == '\n') || (c == '\r');
}

int is_spacing(char c) {
    return (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f');
}

int is_whitespace(char c) {
    return is_spacing(c) || is_end_of_line(c);
}

int is_letter(char c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_alphanumeric(char c) {
    return is_letter(c) || is_digit(c);
}

int is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

int string_compare(char *a, char *b) {
    while (*a && *b && *a == *b) {
        a += 1;
        b += 1;
    }
    return *a - *b;
}

int string_compare_1l(char *a, char *b, int b_length) {
    int result, i;
    i = 0;
    while (*a && i < b_length && *a == b[i]) {
        a += 1;
        i += 1;
    }
    if (*a == '\0' && i == b_length) {
        result = 0;
    } else if (i == b_length) {
        result = *a;
    } else {
        result = *a - b[i];
    }
    return result;
}

int string_to_int(char *str, int len) {
    // the maximum characters a 32-bit integer can have, is 11
    char null_terminated_str[12] = {0};
    MemoryCopy(null_terminated_str, str, len);
    int i = atoi(null_terminated_str);
    return i;
}

float string_to_float(char *str, int len) {
    // 1 sign character + largest integral part: 39 + 1 decimal point + fractional part: 7 = 48
    char null_terminated_str[49] = {0};
    MemoryCopy(null_terminated_str, str, len);
    float f = (float)atof(null_terminated_str);
    return f;
}

Arena arena_create(Byte *backing_buffer, S64 backing_buffer_size) {
    return (Arena){backing_buffer, backing_buffer_size, 0};
}

// NOTE(jan): There are three growth strategies for the backing buffer of arena:
// 0. Don't grow at all. Sets an upper limit to possible memory usage.
// 1. Allocated huge amount of virtual address space and only commit memory as needed
// 2. Grow it like a dynamic array.
// 3. Chain new memory blocks at the end of the old one like a linked list. (Linked lists no problem for performance in
// this context as the linked list is expected to always be pretty small.)
void *arena_alloc_align(Arena *arena, S64 len, Byte alignment) {
    intptr_t misaligned_addr = (intptr_t)arena->buffer + arena->offset;
    intptr_t aligned_addr = (misaligned_addr + (alignment - 1)) & ~(alignment - 1);
    assert(aligned_addr + len <= (intptr_t)arena->buffer + arena->buffer_len);
    arena->offset = (aligned_addr - (intptr_t)arena->buffer) + len;
    return (Byte*)aligned_addr;
}

void *arena_alloc(Arena *arena, S64 len) {
    return arena_alloc_align(arena, len, 2*sizeof(void*)); // 16 byte alignment
}

void arena_free_all(Arena *arena) {
    arena->offset = 0;
}

Arena *begin_scratch(void) {
    return &global_scratch_arena;
}

void end_scratch(void) {
    arena_free_all(&global_scratch_arena);
}
