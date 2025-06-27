#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <fstream>
#include <sstream>
#include <stb_image.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

class Window {
private:
    GLFWwindow *window;
    const GLuint width;
    const GLuint height;

public:
    Window(GLuint width, GLuint height, const GLchar* title) : width(width), height(height) {
        glfwInit();
        glfwWindowHint(GLFW_SAMPLES, 8);

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            throw runtime_error("Falha ao criar a janela GLFW");
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw runtime_error("Falha ao inicializar GLAD");
        }

        GLint fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
    }
    ~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLboolean shouldClose() const{
        return glfwWindowShouldClose(window);
    }

    GLvoid swapBuffers() const {
        glfwSwapBuffers(window);
    }

    GLFWwindow* getHandle() const {
        return window;
    }
};

class Shader {
private:
    GLuint ID;
    static constexpr const char* PROGRAM_LINK_ERROR = "ERROR::SHADER::PROGRAM::LINKING_FAILED\n";
public:
    Shader(const GLchar* vertexPath, const GLchar* fragmentPath) {
        ID = createShaderProgram(vertexPath, fragmentPath);
    }
    ~Shader() {
        glDeleteProgram(ID);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setMat4(const GLchar* name, const mat4& matrix) const {
        GLint location = glGetUniformLocation(ID, name);
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(matrix));
    }

    GLuint getProgram() const {
        return ID;
    }
private:
    string readShaderFile(const string& path) const {
        ifstream shaderFile(path);
        if (!shaderFile.is_open()) {
            throw runtime_error("Falha ao abrir o arquivo de shader " + path);
        }

        stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();

        return shaderStream.str();
    }

    GLuint compileShader(const string& source, GLenum type) {
        GLuint shader = glCreateShader(type);
        const GLchar* sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, NULL);
        glCompileShader(shader);

        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            string shaderTypeName = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            throw runtime_error("ERROR::SHADER::" + shaderTypeName + "::COMPILATION_FAILED\n" + infoLog);
        }

        return shader;
    }

    void checkProgramLinkStatus(GLuint program) {
        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            throw runtime_error(string(PROGRAM_LINK_ERROR) + infoLog);
        }
    }

    GLuint createShaderProgram(const GLchar* vertexPath, const GLchar* fragmentPath) {
        string vertexCode = readShaderFile(vertexPath);
        string fragmentCode = readShaderFile(fragmentPath);

        GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        checkProgramLinkStatus(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }
};

class TileMap {
private:
    vector<Tile> tileset;
public:
};