bool read_file(wchar_t *path_to_file, byte *data, int data_size, int *bytes_read) {
    bool success = false;
    HANDLE file_handle = CreateFile(path_to_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER large_int;
        GetFileSizeEx(file_handle, &large_int);
        int file_size = (int)large_int.QuadPart;
        if (file_size <= data_size) {
            success = (bool)ReadFile(file_handle, data, file_size, (DWORD *)bytes_read, NULL);
        }
    }
    return success;
}
