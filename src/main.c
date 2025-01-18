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
#include "basic_math.c"
#include "os_basic.c"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

// Windows OpenGL Functions
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

// OpenGL Functions
PFNGLGETSTRINGIPROC glGetStringi;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDEBUGMESSAGEINSERTPROC glDebugMessageInsert;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;

LARGE_INTEGER frequency = {0};

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
            // global_window_active = wparam;
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

void APIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar*
                                  message, const void* userParam) {
    OutputDebugStringA(message);
    assert(severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_NOTIFICATION);
}

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
    //
    // Create a scratch arena.
    int scratch_buffer_len = Megabytes(2);
    byte *scratch_buffer = VirtualAlloc(NULL, scratch_buffer_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (scratch_buffer == NULL) {
        MessageBox(NULL, L"Fatal: Failed to allocate memory.", L"Error", MB_ICONEXCLAMATION);
        ExitProcess(1);
    }
    Arena scratch = arena_create(scratch_buffer, scratch_buffer_len);

    //
    // Query frequency.
    QueryPerformanceFrequency(&frequency);

    //
    // Create window class.
    WNDCLASS window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = window_callback,
        .hInstance = instance,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"WindowClass",
    };
    RegisterClass(&window_class);

    //
    // Temporary window creation and wgl extension function loading.
    int pixel_format;
    bool wgl_arb_create_context_supported = false;
    bool wgl_arb_create_context_profile_supported = false;
    {
        HWND temp_window = CreateWindowEx(0, window_class.lpszClassName, L"Window", 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                          CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);

        // Choosing the pixel format like is makes it likely to receive a hardware accelerated context.
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

        pixel_format = ChoosePixelFormat(dc, &pfd);
        SetPixelFormat(dc, pixel_format, &pfd);

        // Create OpenGL context and make it current.
        HGLRC rc = wglCreateContext(dc);
        wglMakeCurrent(dc, rc);

        // Query version and output the OpenGL version.
        char *version_string = glGetString(GL_VERSION);
        wchar_t version_string_out[128];
        swprintf(version_string_out, 128, L"Dummy Context OpenGL version: %S\n", version_string);
        OutputDebugString(version_string_out);

        // Get major version number
        char *at_version = version_string;
        while (*at_version != '.') { at_version += 1; }
        int major_string_length = (int)(at_version - version_string);
        int major_version = string_to_int(version_string, major_string_length);
        if (major_version < 3) {
            MessageBox(NULL, L"OpenGL version is too low and not supported. Aborting.", L"Error", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        bool wgl_pixel_format_supported = false;

        wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

        if (wglGetExtensionsStringARB) {
            OutputDebugString(L"WGL_ARB_extensions_string is supported.\n");

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

                    if (0 == string_compare_1l("WGL_ARB_pixel_format", at, extension_name_length)) {
                        OutputDebugString(L"WGL_ARB_pixel_format is supported.\n");
                        wgl_pixel_format_supported = true;
                    } else if (0 == string_compare_1l("WGL_ARB_create_context", at, extension_name_length)) {
                        OutputDebugString(L"WGL_ARB_create_context is supported.\n");
                        wgl_arb_create_context_supported = true;
                    } else if (0 == string_compare_1l("WGL_ARB_create_context_profile", at, extension_name_length)) {
                        OutputDebugString(L"WGL_ARB_create_context_profile is supported.\n");
                        wgl_arb_create_context_profile_supported = true;
                    }

                    at = end;
                }
            }
        } else {
            MessageBox(NULL, L"WGL_ARB_extensions_string is not supported. Aborting.", L"Error", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        if (!wgl_pixel_format_supported) {
            MessageBox(NULL, L"WGL_ARB_pixel_format is not supported. Aborting.", L"ERROR", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        // Load wgl functions here.
        wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        // Attributes for new OpenGL context.
        int attribute_list[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            // WGL_SAMPLES_ARB,        GL_TRUE, // this is for multisampling, check for WGL_ARB_multisample extension
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_ALPHA_BITS_ARB,     8,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            0
        };

        UINT num_formats;

        // Get special format, store index in pixel_format. Required for SetPixelFormat in new window.
        BOOL pixel_format_available = wglChoosePixelFormatARB(dc, attribute_list, NULL, 1, &pixel_format, &num_formats);

        if (!pixel_format_available || num_formats == 0) {
            MessageBox(NULL, L"No fitting pixel format available. Aborting.", L"Error", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        wglMakeCurrent(dc, NULL);
        wglDeleteContext(rc);
    }

    //
    // Window and OpenGL context creation and set up.
    HWND window;
    HDC dc;
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

        dc = GetDC(window);

        PIXELFORMATDESCRIPTOR pfd;
        SetPixelFormat(dc, pixel_format, &pfd);

        HGLRC rc;
        if (wgl_arb_create_context_supported) {
            int attrib_list_insert_index = 4;
            int attrib_list[9] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4, // OpenGL Context Major Version
                WGL_CONTEXT_MINOR_VERSION_ARB, 3, // OpenGL Context Minor Version
                0, 0, // reserved for potential debug flag
                0, 0, // reserved for potential core profile
                0,
            };
            if (IsEnabled(DEBUG)) {
                attrib_list[attrib_list_insert_index++] = WGL_CONTEXT_FLAGS_ARB;
                attrib_list[attrib_list_insert_index++] = WGL_CONTEXT_DEBUG_BIT_ARB;
            }
            if (wgl_arb_create_context_profile_supported) {
                attrib_list[attrib_list_insert_index++] = WGL_CONTEXT_PROFILE_MASK_ARB;
                attrib_list[attrib_list_insert_index++] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            }
            rc = wglCreateContextAttribsARB(dc, NULL, attrib_list);
        } else {
            rc = wglCreateContext(dc);
        }

        wglMakeCurrent(dc, rc);

        char *version_string = glGetString(GL_VERSION);
        wchar_t version_string_out[128];
        swprintf(version_string_out, 128, L"OpenGL version: %S\n", version_string);
        OutputDebugString(version_string_out);

        int context_flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        if (IsEnabled(DEBUG) && !(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
            MessageBox(NULL, L"Debug build is specified but OpenGL context does not have the debug flag set.",
                       L"Warning", MB_ICONEXCLAMATION);
        } else if (!IsEnabled(DEBUG) && context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            MessageBox(NULL, L"Non-debug build is specified but OpenGL context has the debug flag set.", L"Warning",
                       MB_ICONEXCLAMATION);
        }

        // Check for GL extension support.
        int num_extensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        wchar_t num_extensions_string[128];
        swprintf(num_extensions_string, 128, L"OpenGL extensions available: %d\n", num_extensions);
        OutputDebugString(num_extensions_string);

        // glGetStringi is core in 3.0 so no checking required since we get a 3.3 core context atm
        glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

        // Check for GL extensions here.
        for (int i = 0; i < num_extensions; ++i) {
            char *extension_name = glGetStringi(GL_EXTENSIONS, i);
            if (0) {}
            else if (0 == string_compare(extension_name, "...")) { /* ... */ }
        }

        // Load gl functions here.
        glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
        glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
        glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
        glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
        glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)wglGetProcAddress("glDebugMessageInsert");
        glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
        glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
        glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
        glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
        glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)wglGetProcAddress("glValidateProgram");
        glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
        glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
        glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
        glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
        glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    }

    //
    // Enable OpenGL debugging when debug build is defined.
    if (IsEnabled(DEBUG)) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_message_callback, NULL);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1,
                             "OpenGL Debug Message Callback is enabled.\n");
    }

    //
    // OpenGL shader setup.
    GLuint default_program;
    {
        int file_size;
        Byte *file_text;

        GLuint vertex_shader, fragment_shader;

        bool vertex_read_success = read_file(&scratch, L"..\\src\\default_vertex.glsl", &file_text, &file_size);
        if (vertex_read_success) {
            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex_shader, 1, (char **)&file_text, &file_size);
            glCompileShader(vertex_shader);
        } else {
            MessageBox(NULL, L"Fatal: Failed to read ..\\src\\default_vertex.glsl.", L"Error", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        bool fragment_read_success = read_file(&scratch, L"..\\src\\default_fragment.glsl", &file_text, &file_size);
        if (fragment_read_success) {
            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment_shader, 1, (char **)&file_text, &file_size);
            glCompileShader(fragment_shader);
        } else {
            MessageBox(NULL, L"Fatal: Failed to read ..\\src\\default_fragment.glsl.", L"Error", MB_ICONEXCLAMATION);
            ExitProcess(1);
        }

        default_program = glCreateProgram();
        glAttachShader(default_program, vertex_shader);
        glAttachShader(default_program, fragment_shader);
        glLinkProgram(default_program);

        glDetachShader(default_program, vertex_shader);
        glDetachShader(default_program, fragment_shader);

        glValidateProgram(default_program);
        GLint linked = GL_FALSE;
        glGetProgramiv(default_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char *vertex_errors = arena_alloc(&scratch, 1024);
            char *fragment_errors = arena_alloc(&scratch, 1024);
            char *program_errors = arena_alloc(&scratch, 1024);
            GLsizei log_length;
            glGetShaderInfoLog(vertex_shader, 1024, &log_length, vertex_errors);
            if (log_length > 0) {
                OutputDebugString(L"Vertex shader had some errors:\n");
                OutputDebugStringA(vertex_errors);
            }
            glGetShaderInfoLog(fragment_shader, 1024, &log_length, fragment_errors);
            if (log_length > 0) {
                OutputDebugString(L"Fragment shader had some errors:\n");
                OutputDebugStringA(fragment_errors);
            }
            glGetProgramInfoLog(default_program, 1024, &log_length, program_errors);
            if (log_length > 0) {
                OutputDebugString(L"Program linking had some errors:\n");
                OutputDebugStringA(program_errors);
            }
            assert(0 && "There are linking errors.");
            ExitProcess(1);
        }

        arena_free_all(&scratch);
    }

    //
    // OpenGL vertex specification.
    GLuint vertex_array;
    {
        glGenVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);

        GLuint vertex_buffer, index_buffer;
        glGenBuffers(1, &vertex_buffer);
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    }

    //
    // Main loop preparation.
    ShowWindow(window, SW_SHOWNORMAL);

    double time_begin, time_end;
    time_begin = 0.0;
    time_end = 0.0;

    DWORD current_width = 0;
    DWORD current_height = 0;

    bool close = false;

    //
    // Main loop.
    while (!close) {
        time_begin = time_end;
        time_end = get_time_in_seconds();
        double delta_time = time_end - time_begin;
        delta_time;

        // Window Message Loop
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                close = true;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Handle Input.
        // TODO

        // Create/Recreate swap chain on start-up/resize.
        RECT client_rect;
        DWORD width, height;

        GetClientRect(window, &client_rect);

        width = client_rect.right;
        height = client_rect.bottom;

        if (width != current_width || height != current_height) {
            // TODO
        }

        // Render.
        {
            glClearColor(0.09f, 0.08f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            SwapBuffers(dc);
        }
    }

    ExitProcess(0);
}
