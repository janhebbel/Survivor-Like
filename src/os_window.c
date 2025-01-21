#define UNICODE
#include <windows.h>

#include <GL/gl.h>
#include "GL/glext.h"
#include "GL/glcorearb.h"
#include "GL/wglext.h"

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
PFNGLNAMEDBUFFERDATAPROC glNamedBufferData;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

// Types
typedef struct OS_Window OS_Window;
struct OS_Window {
    HWND handle;
    HDC context;
    struct {
        bool is_down[256];
        bool alt_down[256];
        bool was_down[256];
    };
    bool close;
};

typedef struct OS_Rect OS_Rect;
struct OS_Rect {
    int width;
    int height;
};

bool os_key_is_down(OS_Window *window, U8 key) {
    return window->is_down[key];
}

bool os_key_is_down_initial(OS_Window *window, U8 key) {
    return window->is_down[key] && !window->was_down[key];
}

void os_show_window(OS_Window *window) {
    ShowWindow(window->handle, SW_SHOWNORMAL);
}

OS_Rect os_get_framebuffer_size(OS_Window *window) {
    RECT client_rect;
    GetClientRect(window->handle, &client_rect);
    OS_Rect rect = {(int)client_rect.right, (int)client_rect.bottom};
    return rect;
}

bool os_window_should_close(OS_Window *window) {
    return window->close;
}

void os_swap_buffers(OS_Window *window) {
    SwapBuffers(window->context);
}

void APIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar
                                  *message, const void *userParam) {
    os_debug_print(L"%S\n", message);
    assert(severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_NOTIFICATION);
}

LRESULT WINAPI os_window_callback(HWND handle, UINT message, WPARAM wparam, LPARAM lparam) {
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
            assert(0 && "This should be unreachable.");

        default:
            result = DefWindowProc(handle, message, wparam, lparam);
            break;
    }

    return result;
}

void os_process_events(OS_Window *window) {
    // Update key_was_down every frame.
    for (int i = 0; i < 256; ++i) {
        window->was_down[i] = window->is_down[i];
    }

    // Message loop.
    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
        switch (message.message) {
            case WM_QUIT:
                window->close = true;
                break;

            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                U16 key_code     = LOWORD(message.wParam);
                U16 key_flags    = HIWORD(message.lParam);
                // U16 repeat_count = LOWORD(message.lParam);

                bool is_up    = (key_flags & KF_UP)      == KF_UP;
                bool alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;
                // bool was_down = (key_flags & KF_REPEAT)  == KF_REPEAT;

                window->is_down[key_code]  = !is_up;
                window->alt_down[key_code] = alt_down;

                if (key_code == VK_F4 && alt_down && !is_up) {
                    PostMessage(window->handle, WM_CLOSE, 0, 0);
                }
                break;

            default:
                TranslateMessage(&message);
                DispatchMessage(&message);
        }
    }
}

bool os_init_opengl(HDC context) {
    //
    // Temporary window creation and wgl extension function loading.
    int pixel_format;
    bool wgl_arb_create_context_supported = false;
    bool wgl_arb_create_context_profile_supported = false;
    {
        // NOTE(jan): L"WindowClass" here is hard-coded and only correct because the other class name is also
        // hard-coded.
        HWND temp_window = CreateWindowEx(0, L"WindowClass", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                          CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);

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
        os_debug_print(L"Dummy context OpenGL version: %S\n", version_string);

        // Get major version number
        char *at_version = version_string;
        while (*at_version != '.') { at_version += 1; }
        int major_string_length = (int)(at_version - version_string);
        int major_version = string_to_int(version_string, major_string_length);
        if (major_version < 3) {
            os_notification_window(L"Error", L"This OpenGL version is too old and not supported.");
            return false;
        }

        bool wgl_pixel_format_supported = false;

        wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

        if (wglGetExtensionsStringARB) {
            os_debug_print(L"WGL_ARB_extensions_string is supported.\n");

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
                        os_debug_print(L"WGL_ARB_pixel_format is supported.\n");
                        wgl_pixel_format_supported = true;
                    } else if (0 == string_compare_1l("WGL_ARB_create_context", at, extension_name_length)) {
                        os_debug_print(L"WGL_ARB_create_context is supported.\n");
                        wgl_arb_create_context_supported = true;
                    } else if (0 == string_compare_1l("WGL_ARB_create_context_profile", at, extension_name_length)) {
                        os_debug_print(L"WGL_ARB_create_context_profile is supported.\n");
                        wgl_arb_create_context_profile_supported = true;
                    }

                    at = end;
                }
            }
        } else {
            os_notification_window(L"Error", L"WGL_ARB_extensions_string is not supported. Aborting.");
            return false;
        }

        if (!wgl_pixel_format_supported) {
            os_notification_window(L"Error", L"WGL_ARB_pixel_format is not supported. Aborting.");
            return false;
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
            os_notification_window(L"Error", L"No fitting pixel format available. Aborting.");
            return false;
        }

        wglMakeCurrent(dc, NULL);
        wglDeleteContext(rc);
    }

    //
    // Window and OpenGL context creation and set up.
    {
        PIXELFORMATDESCRIPTOR pfd;
        SetPixelFormat(context, pixel_format, &pfd);

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
            rc = wglCreateContextAttribsARB(context, NULL, attrib_list);
        } else {
            rc = wglCreateContext(context);
        }

        wglMakeCurrent(context, rc);

        char *version_string = glGetString(GL_VERSION);
        os_debug_print(L"OpenGL version: %S\n", version_string);

        int context_flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        if (IsEnabled(DEBUG) && !(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
            os_notification_window(L"Warning",
                                   L"Debug build is specified but OpenGL context does not have the debug flag set.");
        } else if (!IsEnabled(DEBUG) && context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            os_notification_window(L"Warning",
                                   L"Non-debug build is specified but OpenGL context has the debug flag set.");
        }

        // Check for GL extension support.
        int num_extensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        os_debug_print(L"OpenGL extensions available: %d\n", num_extensions);

        // glGetStringi is core in 3.0 so no checking required since we get a 3.3 core context atm
        glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

        // Check for GL extensions here.
        for (int i = 0; i < num_extensions; ++i) {
            char *extension_name = glGetStringi(GL_EXTENSIONS, i);
            if      (0 == string_compare(extension_name, "...")) { /* ... */ }
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
        glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)wglGetProcAddress("glNamedBufferData");
        glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
        glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
        glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
        glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    }

    //
    // Enable OpenGL debugging when debug build is defined.
    if (IsEnabled(DEBUG)) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_message_callback, NULL);
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1,
                             "OpenGL Debug Message Callback is enabled.");
    }

    return true;
}

OS_Window *os_create_window(Arena *persistent, wchar_t *title, int width, int height) {
    //
    // Create window class.
    HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASS window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = os_window_callback,
        .hInstance = instance,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"WindowClass",
    };
    RegisterClass(&window_class);

    DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    DWORD window_style_ex = 0;

    RECT window_rect = {0};
    window_rect.right = width;
    window_rect.bottom = height;
    AdjustWindowRectEx(&window_rect, window_style, FALSE, window_style_ex);

    OS_Window window = {0};
    window.handle = CreateWindowEx(window_style_ex, window_class.lpszClassName, title, window_style, CW_USEDEFAULT,
                                   CW_USEDEFAULT, window_rect.right - window_rect.left, window_rect.bottom -
                                   window_rect.top, NULL, NULL, instance, NULL);
    window.context = GetDC(window.handle);

    if (!os_init_opengl(window.context)) {
        os_notification_window(L"Error", L"Fatal: Failed to initialize OpenGL rendering backend.");
        return NULL;
    }

    OS_Window *result_window = (OS_Window*)arena_alloc(persistent, sizeof(OS_Window));
    *result_window = window;
    return result_window;
}

int entry_point();

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
    return entry_point();
}
