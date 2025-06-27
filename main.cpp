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

struct Tile {
    GLuint VAO;
    GLuint texID;
    GLint iTile;
    vec3 position;
    vec3 dimensions;
    GLfloat ds, dt;
    GLboolean caminhavel;
};

class Window {
private:
    GLFWwindow *window;
    const GLuint width;
    const GLuint height;

public:
    Window(GLuint width, GLuint height, const GLchar *title) : width(width), height(height) {
        glfwInit();
        glfwWindowHint(GLFW_SAMPLES, 8);

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            throw runtime_error("Falha ao criar a janela GLFW");
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
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

    GLboolean shouldClose() const {
        return glfwWindowShouldClose(window);
    }

    GLvoid swapBuffers() const {
        glfwSwapBuffers(window);
    }

    GLFWwindow *getHandle() const {
        return window;
    }
};

class Shader {
private:
    GLuint ID;
    static constexpr const char *PROGRAM_LINK_ERROR = "ERROR::SHADER::PROGRAM::LINKING_FAILED\n";

public:
    Shader(const GLchar *vertexPath, const GLchar *fragmentPath) {
        ID = createShaderProgram(vertexPath, fragmentPath);
    }

    ~Shader() {
        glDeleteProgram(ID);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setMat4(const GLchar *name, const mat4 &matrix) const {
        GLint location = glGetUniformLocation(ID, name);
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(matrix));
    }

    GLuint getProgram() const {
        return ID;
    }

private:
    string readShaderFile(const string &path) const {
        ifstream shaderFile(path);
        if (!shaderFile.is_open()) {
            throw runtime_error("Falha ao abrir o arquivo de shader " + path);
        }

        stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();

        return shaderStream.str();
    }

    GLuint compileShader(const string &source, GLenum type) {
        GLuint shader = glCreateShader(type);
        const GLchar *sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, NULL);
        glCompileShader(shader);

        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            string shaderTypeName = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            cerr << "Shader source: " << source << endl;
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

    GLuint createShaderProgram(const GLchar *vertexPath, const GLchar *fragmentPath) {
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
    static constexpr int TILEMAP_WIDTH = 3;
    static constexpr int TILEMAP_HEIGHT = 3;

    vector<Tile> tileset;
    int map[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    GLuint textID;

public:
    TileMap(const string &tilesetPath) {
        int imgWidth, imgHeight;
        textID = loadTexture(tilesetPath, imgWidth, imgHeight);

        initializeTileset();

        initializeMap();
    }

    GLint getHeight() const {
        return TILEMAP_HEIGHT;
    }

    GLint getWidth() const {
        return TILEMAP_WIDTH;
    }

    vector<Tile> getTileset() const {
        return tileset;
    }

    const int *operator[](int index) const {
        return map[index];
    }

    GLvoid draw(const Shader &shader) const {
        float x0 = 400.0f;
        float yo = 100.0f;

        for (int i = 0; i < TILEMAP_HEIGHT; i++) {
            for (int j = 0; j < TILEMAP_WIDTH; j++) {
                cout << "Drawing tile at i=" << i << ", j=" << j << endl;

                int tileIndex = map[i][j];
                if (tileIndex >= tileset.size()) {
                    cout << "Tile index out of range" << tileIndex << endl;
                    continue;
                }

                Tile currentTile = tileset[tileIndex];

                // Calculate isometric position
                float x = x0 + (j - i) * currentTile.dimensions.x / 2.0f;
                float y = yo + (j + i) * currentTile.dimensions.y / 2.0f;

                // Create model matrix
                mat4 model(1.0f);
                model = translate(model, vec3(x, y, 0.0f));
                model = scale(model, currentTile.dimensions);

                //Set uniforms
                shader.setMat4("model", model);
                glUniform2f(glGetUniformLocation(shader.getProgram(), "offsetTex"),
                            currentTile.iTile * currentTile.ds, 0.0f);

                glBindVertexArray(currentTile.VAO);
                glBindTexture(GL_TEXTURE_2D, currentTile.texID);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        cout << "Finished drawing map" << endl;
    }

    GLboolean isWalkable(int x, int y) const {
        if (x < 0 || x >= TILEMAP_WIDTH || y < 0 || y >= TILEMAP_HEIGHT) {
            return false;
        }
        return tileset[map[y][x]].caminhavel;
    }

private:
    GLvoid initializeTileset() {
        cout << "Initializing tileset..." << endl;
        tileset.clear();
        for (int i = 0; i < 7; i++) {
            Tile tile;
            tile.dimensions = vec3(114, 57, 1.0);
            tile.iTile = i;
            tile.texID = textID;
            tile.VAO = setupTile(7, tile.ds, tile.dt);
            tile.caminhavel = true;
            tileset.push_back(tile);
            cout << "Tile " << i << " with ds=" << tile.ds << " dt=" << tile.dt << endl;
        }
        tileset[4].caminhavel = false; //agua
        cout << "Tileset initialization complete. Total tiles: " << tileset.size() << endl;
    }

    GLvoid initializeMap() {
        GLint defaultMap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {
            {1, 1, 4},
            {4, 1, 4},
            {4, 4, 1},
        };

        for (int i = 0; i < TILEMAP_HEIGHT; i++) {
            for (int j = 0; j < TILEMAP_WIDTH; j++) {
                map[i][j] = defaultMap[i][j];
            }
        }
    }

    GLuint setupTile(int nTiles, float &ds, float &dt) {
        ds = 1.0f / static_cast<float>(nTiles);
        dt = 1.0f;

        float th = 1.0, tw = 1.0;

        GLfloat vertices[] = {
            // x       y       z    s       t
            0.0f, th / 2.0f, 0.0f, 0.0f, dt / 2.0f, // A (topo)
            tw / 2.0f, th, 0.0f, ds / 2.0f, dt, // B (direita)
            tw / 2.0f, 0.0f, 0.0f, ds / 2.0f, 0.0f, // D (base)
            tw, th / 2.0f, 0.0f, ds, dt / 2.0f // C (esquerda)
        };


        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return VAO;
    }

    GLuint loadTexture(const string &path, int &width, int &height) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        int nrChannels;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            cout << "Texture loaded successfully: " << path << " (" << width << "x" << height << ")" << endl;
        } else {
            cerr << "Failed to load texture: " << path << endl;
            cerr << "STB Error: " << stbi_failure_reason() << endl;
            throw runtime_error("Falha ao carregar a imagem " + path);
        }

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }
};

class Player {
private:
    vec2 position;
    const TileMap &tileMap;

public:
    Player(const TileMap &map) : tileMap(map), position(0, 0) {
    }

    GLvoid handleInput(int key, int action) {
        vec2 aux = position;

        if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            if (position.x > 0) position.x--;
            if (position.y > 0) position.y--;
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            if (position.x > 0) position.x--;
            if (position.y <= tileMap.getHeight() - 2) position.y++;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            if (position.x <= tileMap.getWidth() - 2) position.x++;
            if (position.y <= tileMap.getHeight() - 2) position.y++;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS) {
            if (position.x <= tileMap.getWidth() - 2) position.x++;
            if (position.y > 0) position.y--;
        }

        int x = static_cast<int>(position.x);
        int y = static_cast<int>(position.y);
        if (!tileMap.isWalkable(x, y)) {
            position = aux;
        }
    }

    GLvoid draw(const Shader &shader) const {
        Tile current_tile = tileMap.getTileset()[6];

        GLfloat x0 = 400.0f;
        GLfloat y0 = 100.0f;

        GLfloat x = x0 + (position.x - position.y) * current_tile.dimensions.x / 2.0f;
        GLfloat y = y0 + (position.x + position.y) * current_tile.dimensions.y / 2.0f;

        mat4 model = mat4(1.0f);
        model = translate(model, vec3(x, y, 0.0f));
        model = scale(model, current_tile.dimensions);
        shader.setMat4("model", model);

        vec2 offsetTex;
        offsetTex.s = current_tile.iTile * current_tile.ds;
        offsetTex.t = 0.0f;
        glUniform2f(glGetUniformLocation(shader.getProgram(), "offsetTex"), offsetTex.s, offsetTex.t);

        glBindVertexArray(current_tile.VAO);
        glBindTexture(GL_TEXTURE_2D, current_tile.texID);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class Game {
private:
    Window window;
    Shader shader;
    TileMap tileMap;
    Player player;

public:
    Game(): window(800, 600, "Game"),
            shader("shaders/vertex.vert", "shaders/fragment.frag"),
            tileMap("assets/tilesetIso.png"),
            player(tileMap) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(shader.getProgram(), "tex_buff"), 0);

        mat4 projection = ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
        shader.setMat4("projection", projection);

        glfwSetWindowUserPointer(window.getHandle(), this);
        glfwSetKeyCallback(window.getHandle(), &Game::keyCallback);
    }

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        Game *game = static_cast<Game *>(glfwGetWindowUserPointer(window));
        if (game) {
            game->player.handleInput(key, action);
        }
    }

    void run() {
        double lastTime = glfwGetTime();
        const double targetFPS = 60.0;
        const double frameTime = 1.0 / targetFPS;

        while (!window.shouldClose()) {
            double currentTime = glfwGetTime();
            double deltaTime = currentTime - lastTime;
            if (deltaTime >= frameTime) {
                exitGame(window);
                render();
                window.swapBuffers();
                lastTime = currentTime;
            }
            glfwPollEvents();
        }
    }

private:
    GLvoid render() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glUniform1i(glGetUniformLocation(shader.getProgram(), "tex_buff"), 0);
        tileMap.draw(shader);
        player.draw(shader);
    }

    GLvoid exitGame(Window &window) {
        if (glfwGetKey(window.getHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window.getHandle(), true);
    }
};

int main() {
    try {
        Game game;
        game.run();
    } catch (const exception &e) {
        cerr << "ERROR: " << e.what() << endl;
        return -1;
    }

    return 0;
}
