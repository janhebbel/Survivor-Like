#define UNICODE
#include <windows.h>
#include <GL/gl.h>
#include "GL/glext.h"
#include "GL/glcorearb.h"
#include "GL/wglext.h"

#include <wchar.h>
#include <stdbool.h>
#include <assert.h>

#include "basic.c"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

#define array_len(x) (sizeof(x) / sizeof(*x))

// Windows OpenGL Functions
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;

// OpenGL Functions
PFNGLGETSTRINGIPROC glGetStringi;

bool global_window_active = false;

LARGE_INTEGER frequency = {0};

void fatal_error(wchar_t *message) {
    MessageBox(NULL, message, L"Error", MB_ICONEXCLAMATION);
    ExitProcess(1);
}

double get_time_in_seconds(void) {
    LARGE_INTEGER counter = {0};
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

LRESULT WINAPI window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_ACTIVATEAPP:
            // ClearKeyboardState();
            global_window_active = wparam;
            // ShowCursor(GlobalWindowActive == false);
            break;

        case WM_QUIT: // When PostQuitMessage(0) is called
            assert(0 && "This should be impossible.");
            break;

        case WM_DESTROY: // When DestroyWindow is called
            assert(0 && "This should be impossible.");
            break;

        case WM_CLOSE: // When window's X is pressed.
            // DestroyWindow(window); // ~5 ms for this
            PostQuitMessage(0);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            WORD key_code  = LOWORD(wparam);
            WORD key_flags = HIWORD(lparam);

            bool was_down = (key_flags & (1 << 30)) != 0;
            bool down = (key_flags & (1 << 31)) == 0;

            if (down != was_down) {
                bool alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;

                if (key_code == VK_F11 && down) {
                    // toggle_fullscreen(window);
                }

                if (key_code == VK_F4 && alt_down && down) {
                    PostMessage(window, WM_CLOSE, 0, 0);
                }

                // key_event(key_code, down);
            }

            break;

        default:
            result = DefWindowProc(window, message, wparam, lparam);
            break;
    }

    return result;
}

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
    QueryPerformanceFrequency(&frequency);

    WNDCLASS window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = window_callback,
        .hInstance = instance,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"WindowClass",
    };

    if (RegisterClass(&window_class) == 0) {
        wchar_t error_message[512];
        wsprintf(error_message, L"RegisterClass failed with error code: 0x%lx\n", GetLastError());
        OutputDebugString(error_message);
        fatal_error(error_message);
    }

    // Temporary Window Creation
    {
        HWND temp_window = CreateWindowEx(0, window_class.lpszClassName, L"Window", 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                          CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
        if (temp_window == NULL) {
            wchar_t error_message[512];
            wsprintf(error_message, L"CreateWindowEx failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 24,
            .cAlphaBits = 8,
            .cDepthBits = 24,
            .cStencilBits = 8,
            .iLayerType = PFD_MAIN_PLANE,
        };

        HDC dc = GetDC(temp_window);

        int pixel_format = ChoosePixelFormat(dc, &pfd);
        if (pixel_format == 0) {
            wchar_t error_message[512];
            wsprintf(error_message, L"ChoosePixelFormat failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        if (SetPixelFormat(dc, pixel_format, &pfd) == FALSE) {
            wchar_t error_message[512];
            wsprintf(error_message, L"SetPixelFormat failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        HGLRC rc = wglCreateContext(dc);
        if (rc == NULL) {
            wchar_t error_message[512];
            wsprintf(error_message, L"wglCreateContext failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        if (wglMakeCurrent(dc, rc) == FALSE) {
            wchar_t error_message[512];
            wsprintf(error_message, L"wglMakeCurrent failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        // Load extension functions here.
        // Checking for extension support:
        int num_extensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        wchar_t num_extensions_string[512];
        swprintf(num_extensions_string, 512, L"OpenGL extensions available: %d\n", num_extensions);
        OutputDebugString(num_extensions_string);

        wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
        if (wglGetExtensionsStringARB == NULL) {
            // TODO
            assert(0 && "wglGetExtensionStringARB not supported");
        }

        // Check for wgl extensions here.
        char *wgl_extension_string = wglGetExtensionsStringARB(dc);
        {
            char *at = wgl_extension_string;
            char *end = NULL;

            while (*at) {
                while (*at == ' ') at += 1;
                end = at;
                while (*end && *end != ' ') end += 1;

                int extension_name_length = (int)(end - at);

                if (0) {}
                else if (0 != string_compare_1l("WGL_ARB_pixel_format", at, extension_name_length)) { /* TODO */ }

                at = end;
            }
        }

        wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

        // TODO: if this is not supported or check if supported?
        glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

        // Check for GL extensions here.
        for (int i = 0; i < num_extensions; ++i) {
            char *extension_name = glGetStringi(GL_EXTENSIONS, i);
            if (0) {}
            else if (0 == string_compare(extension_name, "TODO")) { /* TODO */ }
        }

        int attribute_list[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_ALPHA_BITS_ARB,     8,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            0
        };

        UINT num_formats;

        if (wglChoosePixelFormatARB(dc, attribute_list, NULL, 1, &pixel_format, &num_formats) == FALSE) {
            wchar_t error_message[512];
            wsprintf(error_message, L"wglChoosePixelFormatARB failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        if (num_formats == 0) {
            wchar_t error_message[512];
            wsprintf(error_message, L"wglChoosePixelFormatARB could not find a fitting pixel format.\n");
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        if (SetPixelFormat(dc, pixel_format, &pfd) == FALSE) {
            wchar_t error_message[512];
            wsprintf(error_message, L"SetPixelFormat failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }

        wglMakeCurrent(dc, NULL);
        wglDeleteContext(rc);
    }

    // Window creation and set up
    HWND window;
    {
        DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_CLIPSIBLINGS;
        DWORD window_style_ex = 0;

        RECT window_rect = {0};
        window_rect.right = DEFAULT_WIDTH;
        window_rect.bottom = DEFAULT_HEIGHT;
        AdjustWindowRectEx(&window_rect, window_style, FALSE, window_style_ex);

        window = CreateWindowEx(window_style_ex, window_class.lpszClassName, L"Window", window_style, CW_USEDEFAULT,
                                CW_USEDEFAULT, window_rect.right - window_rect.left, window_rect.bottom -
                                window_rect.top, NULL, NULL, instance, NULL);
        if (window == NULL) {
            wchar_t error_message[512];
            wsprintf(error_message, L"CreateWindowEx failed with error code: 0x%lx\n", GetLastError());
            OutputDebugString(error_message);
            fatal_error(error_message);
        }
    }

    ShowWindow(window, SW_SHOWNORMAL);

    double time_begin, time_end;
    time_begin = 0.0;
    time_end = 0.0;

    DWORD current_width = 0;
    DWORD current_height = 0;

    bool close = false;

    while (!close) {
        time_begin = time_end;
        time_end = get_time_in_seconds();
        double delta_time = time_end - time_begin;

        // Show frame time in window title.
        wchar_t delta_time_string[32];
        swprintf(delta_time_string, 32, L"%.2f fps\n", 1.0 / delta_time);
        // OutputDebugString(delta_time_string);

        // Window Message Loop
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                close = true;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Create/Recreate swap chain on start-up/resize.
        RECT client_rect;
        DWORD width, height;

        GetClientRect(window, &client_rect);

        width = client_rect.right;
        height = client_rect.bottom;

        if (width != current_width || height != current_height) {
        }

        // Render.
        // TODO.
    }

    ExitProcess(0);
}
