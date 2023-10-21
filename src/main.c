#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "rpmalloc.h"

#include "logka.h"

#define TEAPOT "obj_files/cow.obj"

typedef struct __attribute__((packed)) {
    float x, y, z;
} Vertex;

typedef struct __attribute__((packed)) {
    unsigned int indexes[3];
} Face;

/*  --- Globals ---  */

    Vertex* vertex_buffer;
    Face*   face_buffer;

    int vertex_count;
    int face_count;

    unsigned int program;

    const int    TARGET_FPS = 165;
    const double FRAME_TIME = 1.0 / TARGET_FPS;

/*  --- Globals ---  */

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    (void) source; (void) id; (void) length; (void) userParam; (void) type; (void) severity;
    gldebug("%s - %s", (type == GL_DEBUG_TYPE_ERROR ? "error" : "debug"), message);
}

void error_callback(int error, const char* description) {
    error("GLFW error (%d): %s", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void) window;
    glViewport(0, 0, width, height);
    debug("Window resized: %dx%d", width, height);
}

char* read_entire_file(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        error("Failed to read entire file %s: %s", filepath, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    unsigned long fileSize = ftell(file);
    rewind(file);

    char *buffer = rpmalloc(fileSize + 1);
    if (buffer == NULL) {
        error("Failed to allocate %ld bytes of memory for reading file %s", fileSize + 1, filepath);
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize) {
        rpfree(buffer);
        fclose(file);
        error("Read less bytes than size of file %s, expected a plain text file.", filepath);
        return NULL;
    }
    buffer[bytesRead] = '\0';

    fclose(file);

    return buffer;
}

unsigned int init_shader(unsigned int type, const char* filename) {
    unsigned int result = glCreateShader(type);
    char* code = read_entire_file(filename);

    if(!code) {
        error("Could not read source code file %s for shader %d. exiting.", filename, type);
        exit(1);
    }

    glShaderSource(result, 1, (const GLchar* const*) &code, NULL);
    glCompileShader(result);

    {
        int compile_status;
        glGetShaderiv(result, GL_COMPILE_STATUS, &compile_status);

        if (compile_status == GL_FALSE) {
            int log_length;
            glGetShaderiv(result, GL_INFO_LOG_LENGTH, &log_length);
            char* message = (char*) alloca(log_length * sizeof(char));
            glGetShaderInfoLog(result, log_length, &log_length, message);
            error("OpenGL: %s: %s", filename, message);
        }
    }

    rpfree(code);

    debug("Loaded shader %d: %s", type, filename);
    return result;
}

void init_shader_program() {
    program = glCreateProgram();

    unsigned int shaders[] = {
        init_shader(GL_VERTEX_SHADER,   "src/vertex.glsl"),
        init_shader(GL_FRAGMENT_SHADER, "src/fragment.glsl")
    };

    #define forshaders() for(unsigned long i = 0; i < sizeof(shaders) / sizeof(unsigned int); i++)

    forshaders() glAttachShader(program, shaders[i]);

    glLinkProgram(program);
    glValidateProgram(program);

    forshaders() glDeleteShader(shaders[i]);
}

int init_uniform(const char* name) {
    int location = glGetUniformLocation(program, name);
    if(location == -1) {
        error("uniform '%s' location was -1", name);
    }
    return location;
}

int main(void) {
    rpmalloc_initialize();

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Utah Teapot", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        error("Failed to initialize GLAD");
        return 1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    info("OpenGL Version: %s", glGetString(GL_VERSION));
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    FILE *file = fopen(TEAPOT, "r");
    if (file == NULL) {
        error("Failed to open file %s: %s", TEAPOT, strerror(errno));
        return 1;
    }

    vertex_count     =  0 ;
    face_count       =  0 ;
    char line[256]   = {0};

    while (fgets(line, sizeof(line), file) != NULL) {
        char first = line[0];
        switch (first) {
            case 'f': {
                face_count++;
            } break;
            case 'v': {
                vertex_count++;
            } break;
            default:
            warn("Unknown obj type ASCII code: %d", (unsigned int) first);
        }
    }

    rewind(file);

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vertex_buffer = (Vertex*) rpmalloc(sizeof(Vertex) * vertex_count);
    face_buffer   = (Face*)   rpmalloc(sizeof(Face)   * face_count);

    unsigned int v_counter = 0;
    unsigned int f_counter = 0;

    if(!vertex_buffer || !face_buffer) { error("malloc failed lmao"); return 1;}

    while (fgets(line, sizeof(line), file) != NULL) {
        char first = line[0];
        switch (first) {
            case 'f': {
                strtok(line, " ");
                char* n1   = strtok(NULL, " ");
                char* n2   = strtok(NULL, " ");
                char* n3   = strtok(NULL, " ");
                face_buffer[f_counter] = (Face) { atoi(n1)-1, atoi(n2)-1, atoi(n3)-1 };
                f_counter++;
            } break;
            case 'v': {
                strtok(line, " ");
                char* n1   = strtok(NULL, " ");
                char* n2   = strtok(NULL, " ");
                char* n3   = strtok(NULL, " ");
                vertex_buffer[v_counter] = (Vertex) { atof(n1), atof(n2), atof(n3) };
                v_counter++;
            } break;
            default:
            warn("Unknown obj type ASCII code: %d", (unsigned int) first);
        }
    }

    fclose(file);

    debug("Loaded %d vertexes, and %d faces.", vertex_count, face_count);

    { // Scale the model down into the viewport
        float max = 0.0f;

        for (int i = 0; i < vertex_count; i++) {
            float x = fabsf(vertex_buffer[i].x);
            float y = fabsf(vertex_buffer[i].y);
            float z = fabsf(vertex_buffer[i].z);
            if (x > max) max = x;
            if (y > max) max = y;
            if (z > max) max = z;
        }

        float scale_factor = 1.2f;

        for (int i = 0; i < vertex_count; i++) {
            vertex_buffer[i].x /= (max * scale_factor);
            vertex_buffer[i].y /= (max * scale_factor);
            vertex_buffer[i].z /= (max * scale_factor);
        }
    }

    { // Center the model vertically on the viewport
        float max_y = 0.0f;
        float min_y = 0.0f;

        for (int i = 0; i < vertex_count; i++) {
            max_y = max_y > vertex_buffer[i].y ? max_y : vertex_buffer[i].y;
            min_y = min_y < vertex_buffer[i].y ? min_y : vertex_buffer[i].y;
        }

        float y_difference = ((1.0f - fabsf(min_y)) - (1.0f - fabsf(max_y))) / 2.0f;

        for (int i = 0; i < vertex_count; i++) {
            vertex_buffer[i].y -= y_difference;
        }
    }

    /* Set up shaders and buffers */

    unsigned int vbuf;
    glGenBuffers(1, &vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertex_buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

    unsigned int ibuf;
    glGenBuffers(1, &ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_count * sizeof(Face), face_buffer, GL_STATIC_DRAW);


    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    init_shader_program();
    glUseProgram(program);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    int user_color = init_uniform("user_color");
    int time = init_uniform("time");


    int rotmat = init_uniform("rotationMatrix");

    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();

        // glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = frameStartTime * M_PI;

        float rotationMatrix[16] = {
            cos(angle), 0, sin(angle), 0,
            0, 1, 0, 0,
            -sin(angle), 0, cos(angle), 0,
            0, 0, 0, 1
        };


        glUniform1f(time, frameStartTime);
        glUniform4f(user_color, 0.0, 0.3, 0.4, 1.0);
        glUniformMatrix4fv(rotmat, 1, GL_FALSE, rotationMatrix);

        glDrawElements(GL_TRIANGLES, face_count * 3, GL_UNSIGNED_INT, NULL);

        glfwSwapBuffers(window);
        glfwPollEvents();

        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - frameStartTime;
        double timeToWait = FRAME_TIME - frameDuration;

        if (timeToWait > 0) {
            glfwWaitEventsTimeout(timeToWait);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    rpfree(vertex_buffer);
    rpfree(face_buffer);

    rpmalloc_global_statistics_t stats;
    rpmalloc_global_statistics(&stats);

    debug(" === Memory allocator stats === ");
    debug("mapped:          %zu", stats.mapped);
    debug("mapped_peak:     %zu", stats.mapped_peak);
    debug("cached:          %zu", stats.cached);
    debug("mapped_total:    %zu", stats.mapped_total);
    debug("unmapped_total:  %zu", stats.unmapped_total);

    rpmalloc_finalize();
    info("Closed program.");
    return 0;
}
