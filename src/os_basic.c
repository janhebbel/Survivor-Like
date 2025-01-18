bool read_file(Arena *arena, wchar_t *path_to_file, Byte **file_data, int *bytes_read) {
    bool success = false;
    HANDLE file_handle = CreateFile(path_to_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER large_int;
        GetFileSizeEx(file_handle, &large_int);
        assert(large_int.QuadPart < U32_MAX); // TODO(jan): handle bigger files when necessary
        int file_size = (int)large_int.QuadPart;
        *file_data = arena_alloc(arena, file_size);
        success = ReadFile(file_handle, *file_data, file_size, (DWORD*)bytes_read, NULL);
    }
    return success;
}
