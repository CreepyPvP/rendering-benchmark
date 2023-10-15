#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vcruntime_string.h>

#include "defines.hpp"
#include "glm/ext/vector_float2.hpp"
#include "shader.hpp"

#define Vao unsigned int

struct Window {
    int width;
    int height;
    GLFWwindow* handle;
};

#define RENDERER_1
#define OBJECTS 10000

static Window globalWindow;
static glm::mat4 projection;

static void updateViewport(int width, int height) {
    glViewport(0, 0, width, height);
}

static void updateProjection() {
    projection = glm::ortho(
        (float) (-globalWindow.width / 2),
        (float) (globalWindow.width / 2),
        (float) (-globalWindow.height / 2),
        (float) (globalWindow.height / 2),
        0.1f,
        100.0f
    );
}

static void frameBufferResizeCallback(GLFWwindow *window, int width, int height) {
    updateViewport(width, height);
    globalWindow.width = width;
    globalWindow.height = height;
    updateProjection();
}

static void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    globalWindow.width = 1280;
    globalWindow.height = 720;
    globalWindow.handle = 
        glfwCreateWindow(
            globalWindow.width, 
            globalWindow.height,
            "Physics",
            NULL,
            NULL
        );
    if (!globalWindow.handle) {
        printf("Failed to create window\n");
    }
    glfwSetFramebufferSizeCallback(globalWindow.handle, frameBufferResizeCallback);
    glfwMakeContextCurrent(globalWindow.handle);
    glfwSwapInterval(0);
}

#ifdef RENDERER_1
static Vao squareVao;

static void setupSquareVao() {
    float vertices[] = {
        -1,  1,
         1,  1,
        -1, -1,
         1, -1,
    };

    unsigned int indices[] = {
        1, 3, 2,
        0, 1, 2,
    };

    GL(glGenVertexArrays(1, &squareVao));
    GL(glBindVertexArray(squareVao));

    unsigned int buffers[2];
    GL(glGenBuffers(2, buffers));
    unsigned int vertexBuffer = buffers[0];
    unsigned int indexBuffer = buffers[1];

    GL(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL(glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * 8,
        vertices,
        GL_STATIC_DRAW
    ));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
    GL(glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(unsigned int) * 6,
        indices,
        GL_STATIC_DRAW
    ));

    GL(glEnableVertexAttribArray(0));
    GL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));
}

#else
static Vao gpuBuffer;

static void setupGpuBuffer() {
    glGenVertexArrays(1, &gpuBuffer);
    glBindVertexArray(gpuBuffer);

    unsigned int buffers[2];
    GL(glGenBuffers(2, buffers));
    unsigned int vertexBuffer = buffers[0];
    unsigned int indexBuffer = buffers[1];

    GL(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * OBJECTS, NULL, GL_DYNAMIC_DRAW));

    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * 6 * OBJECTS, NULL, GL_DYNAMIC_DRAW));

    GL(glEnableVertexAttribArray(0));
    GL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));
}

static void bufferCirlce(float x, float y, float* vertices, unsigned int* indices, int* vertexCount, int* indexCount) {
    int vertexC = *vertexCount;
    int indexC = *indexCount;

    vertices[vertexC + 0] = -1 + x;
    vertices[vertexC + 1] = 1 + y;
    vertices[vertexC + 2] = 1 + x;
    vertices[vertexC + 3] = 1 + y;
    vertices[vertexC + 4] = -1 + x;
    vertices[vertexC + 5] = -1 + y;
    vertices[vertexC + 6] = 1 + x;
    vertices[vertexC + 7] = -1 + y;

    indices[indexC + 0] = 1 + vertexC;
    indices[indexC + 1] = 3 + vertexC;
    indices[indexC + 2] = 2 + vertexC;
    indices[indexC + 3] = 0 + vertexC;
    indices[indexC + 4] = 1 + vertexC;
    indices[indexC + 5] = 2 + vertexC;

    *vertexCount += 8;
    *indexCount += 6;
}

#endif

int main() {
    initWindow();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("failed to load required extensions\n");
    }

    GL(glEnable(GL_BLEND));
    GL(glEnable(GL_MULTISAMPLE));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    updateProjection();

    glm::mat4 view = 
        glm::lookAt(glm::vec3(0, 0, 100), glm::vec3(0), glm::vec3(0, 1, 0));

    CircleShader cirlceShader = createCircleShader(
        "../shader/circleVert.glsl",
        "../shader/circleFrag.glsl"
    );

    const float radius = 15;

    float delta = 0.0f;
    float lastFrame = 0.0f;
    GL(glClearColor(0.1, 0.1, 0.1, 1));


#ifdef RENDERER_1 
    setupSquareVao();
    glm::mat4 nonTranslatedModel = glm::scale(glm::mat4(1), glm::vec3(radius));
#else
    float vertices[OBJECTS * 8];
    unsigned int indices[OBJECTS * 6];
    int vertexCount = 0;
    int indexCount = 0;

    setupGpuBuffer();

    glm::mat4 model = glm::scale(glm::mat4(1), glm::vec3(radius));
#endif

    while (!glfwWindowShouldClose(globalWindow.handle)) {
        if (glfwGetKey(globalWindow.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(globalWindow.handle, true);
        }

        float currentFrame = glfwGetTime();
        delta = currentFrame - lastFrame;
        lastFrame = currentFrame;

        printf("delta: %f\n", delta);
        

        GL(glClear(GL_COLOR_BUFFER_BIT));

#ifdef RENDERER_1
        GL(glUseProgram(cirlceShader.id));
        setUniformMat4(cirlceShader.uProjection, &projection);
        setUniformMat4(cirlceShader.uView, &view);
        GL(glBindVertexArray(squareVao));
        for (int i = 0; i < OBJECTS; ++i) {
            glm::mat4 model = glm::translate(nonTranslatedModel, glm::vec3(0, 0, 0));
            setUniformMat4(cirlceShader.uModel, &model);
            GL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
        }
#else
        vertexCount = 0;
        indexCount = 0;
        for (int i = 0; i < OBJECTS; ++i) {
            bufferCirlce(0, 0, vertices, indices, &vertexCount, &indexCount);
        }

        glBindVertexArray(gpuBuffer);
        
        float* vertexBuffMap = (float*) glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(float) * 8 * OBJECTS, GL_MAP_WRITE_BIT);
        memcpy(vertexBuffMap, vertices, sizeof(float) * 8 * OBJECTS);
        GL(glUnmapBuffer(GL_ARRAY_BUFFER));
        unsigned int* indexBuffMap = (unsigned int*) glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(int) * 6 * OBJECTS, GL_MAP_WRITE_BIT);
        memcpy(indexBuffMap, indices, sizeof(int) * 6 * OBJECTS);
        GL(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));

        GL(glUseProgram(cirlceShader.id));

        GL(setUniformMat4(cirlceShader.uView, &view));
        GL(setUniformMat4(cirlceShader.uProjection, &projection));
        GL(setUniformMat4(cirlceShader.uModel, &model));

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        
#endif

        glfwSwapBuffers(globalWindow.handle);
        glfwPollEvents();
    }
}
