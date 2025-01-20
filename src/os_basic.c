#define UNICODE
#include <windows.h>

double os_get_time_in_seconds(void) {
    LARGE_INTEGER c, f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return (double)c.QuadPart / (double)f.QuadPart;
}

bool os_read_file(wchar_t *path_to_file, Byte **file_data, int *bytes_read) {
    bool success = false;
    Arena *scratch = begin_scratch();
    HANDLE file_handle = CreateFile(path_to_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER large_int;
        GetFileSizeEx(file_handle, &large_int);
        int file_size = (int)large_int.QuadPart;
        *file_data = arena_alloc(scratch, file_size);
        success = ReadFile(file_handle, *file_data, file_size, (DWORD*)bytes_read, NULL);
    }
    end_scratch();
    return success;
}

Byte *os_memory_alloc(S64 bytes) {
    return VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void os_notification_window(wchar_t *title, wchar_t *text) {
    MessageBox(NULL, text, title, MB_ICONEXCLAMATION);
}

void os_debug_print(wchar_t *format, ...) {
    va_list args;
    va_start(args, format);
    Arena *scratch = begin_scratch();

    int len = vswprintf(NULL, 0, format, args);
    wchar_t *out = arena_alloc(scratch, (len + 1) * sizeof(wchar_t));
    vswprintf(out, len + 1, format, args);
    OutputDebugString(out);

    end_scratch();
    va_end(args);
}
