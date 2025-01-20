#include <wchar.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#include "basic.c"
#include "basic_math.c"
#include "os_basic.c"
#include "os_window.c"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

// Types
typedef struct Vertex Vertex;
struct Vertex {
    Vec4F32 position;
    Vec2F32 uv;
    Vec3F32 normal;
};

typedef struct Camera Camera;
struct Camera {
    Vec3F32 position;
    Vec3F32 forward;
    Vec3F32 up;
    F32 fov;
    F32 speed;
    F32 sensitivity;
};

int entry_point(void) {
    //
    // Process memory allocations.
    S64 global_scratch_buffer_len = Kilobytes(16);
    Byte *global_scratch_buffer = os_memory_alloc(global_scratch_buffer_len);
    global_scratch_arena = arena_create(global_scratch_buffer, global_scratch_buffer_len);

    S64 process_memory_len = Megabytes(2);
    Byte *process_memory = os_memory_alloc(process_memory_len);
    Arena persistent = arena_create(process_memory, process_memory_len);

    if (process_memory == NULL || global_scratch_buffer == NULL) {
        os_notification_window(L"Error", L"Failed to allocate the necessary memory for running this process.");
        return 1;
    }

    //
    // Create OS Window.
    OS_Window *window = os_create_window(&persistent);
    if (window == NULL) {
        os_notification_window(L"Error", L"Failed to create a window.");
        return 1;
    }

    //
    // Main loop preparation.
    Camera camera = {
        .position = {0.0f, 0.0f, 5.0f},
        .forward = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fov = 0.18f,
        .speed = 3.0f,
        .sensitivity = 0.0003f,
    };

    //
    // OpenGL shader setup.
    GLuint default_program;
    {
        int file_size;
        Byte *file_text;

        GLuint vertex_shader, fragment_shader;

        bool vertex_read_success = os_read_file(L"..\\src\\default_vertex.glsl", &file_text, &file_size);
        if (vertex_read_success) {
            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex_shader, 1, (char **)&file_text, &file_size);
            glCompileShader(vertex_shader);
        } else {
            os_notification_window(L"Error", L"Fatal: Failed to read ..\\src\\default_vertex.glsl.");
            return 1;
        }

        bool fragment_read_success = os_read_file(L"..\\src\\default_fragment.glsl", &file_text, &file_size);
        if (fragment_read_success) {
            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment_shader, 1, (char **)&file_text, &file_size);
            glCompileShader(fragment_shader);
        } else {
            os_notification_window(L"Error", L"Fatal: Failed to read ..\\src\\default_fragment.glsl.");
            return 1;
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
            Arena *scratch = begin_scratch();
            char *vertex_errors = arena_alloc(scratch, 1024);
            char *fragment_errors = arena_alloc(scratch, 1024);
            char *program_errors = arena_alloc(scratch, 1024);
            GLsizei log_length;
            glGetShaderInfoLog(vertex_shader, 1024, &log_length, vertex_errors);
            if (log_length > 0) {
                os_debug_print(L"Vertex shader had some errors:\n%S\n", vertex_errors);
            }
            glGetShaderInfoLog(fragment_shader, 1024, &log_length, fragment_errors);
            if (log_length > 0) {
                os_debug_print(L"Fragment shader had some errors:\n%S\n", fragment_errors);
            }
            glGetProgramInfoLog(default_program, 1024, &log_length, program_errors);
            if (log_length > 0) {
                os_debug_print(L"Program linking had some errors:\n%S\n", program_errors);
            }
            end_scratch();
            assert(0 && "There are linking errors.");
            return 1;
        }
    }

    //
    // OpenGL vertex specification.
    GLuint vertex_array;
    {
        Vertex vertices[] = {
            {-0.5f, -0.5f, 0.0f, 1.0f}, // bottom left
            { 0.5f, -0.5f, 0.0f, 1.0f}, // bottom right
            { 0.5f,  0.5f, 0.0f, 1.0f}, // top    right
                                        // { 0.5f,  0.5f, 0.0f, 1.0f}, // top    right
            {-0.5f,  0.5f, 0.0f, 1.0f}, // top    left
                                        // {-0.5f, -0.5f, 0.0f, 1.0f}, // bottom left
        };
        uint indices[] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);

        GLuint vertex_buffer, index_buffer;
        glGenBuffers(1, &vertex_buffer);
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        glNamedBufferData(vertex_buffer, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glNamedBufferData(index_buffer, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
    }

    double time_begin, time_end;
    time_begin = 0.0;
    time_end = 0.0;

    os_show_window(window);

    //
    // Main loop.
    while (!os_window_should_close(window)) {
        time_begin = time_end;
        time_end = os_get_time_in_seconds();
        double delta_time = time_end - time_begin;
        delta_time;

        os_process_events(window);

        // Handle Input.
        if (os_key_is_down_initial(window, 'A')) {
            os_debug_print(L"A\n");
        }

        // Update window width and height in case of window resize.
        OS_Rect rect = os_get_framebuffer_size(window);
        int window_width  = rect.width;
        int window_height = rect.height;

        // Render.
        {
            Mat4F32 model = identity_mat4f32(1.0f);
            Mat4F32 view = look_at(camera.position, add_3f32(camera.position, camera.forward), camera.up);
            Mat4F32 proj = perspective(camera.fov, (F32)window_height / window_width, 0.1f, 100.0f);
            Mat4F32 mvp = mul3_mat4f32(proj, view, model);
            glClearColor(0.09f, 0.08f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(default_program);
            glUniformMatrix4fv(0, 1, GL_FALSE, (F32*)mvp.v);
            glViewport(0, 0, window_width, window_height);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        os_swap_buffers(window);
    }

    return 0;
}
