#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "Config.hpp"
#include "Renderer.hpp"
#include "Util.hpp"
#include "XiangqiGame.hpp"
#include "XiangqiRules.hpp"

#include <algorithm>
#include <cmath>
#include <string>


struct InputState {
    bool rmbDown = false;
    double lastX = 0.0;
    double lastY = 0.0;
};

struct App {
    GLFWwindow* window = nullptr;
    int w = 1280;
    int h = 720;

    OrbitCamera cam;
    Renderer renderer;
    XiangqiGame game;

    InputState input;
};

static void updateWindowTitle(GLFWwindow* window, const XiangqiGame& game) {
    static std::string last;
    std::string title = std::string("Xiangqi3D (OpenGL) - ") + game.windowTitleCN();
    if (title != last) {
        glfwSetWindowTitle(window, title.c_str());
        last = std::move(title);
    }
}


static bool pickBoardPos(const OrbitCamera& cam, double mouseX, double mouseY, int w, int h, Pos& outPos) {
    Ray r = cam.screenRay(mouseX, mouseY, w, h);

    const float denom = r.dir.y;
    if (std::abs(denom) < 1e-6f) return false;

    float t = (cfg::BOARD_PLANE_Y - r.origin.y) / denom;
    if (t < 0.0f) return false;

    glm::vec3 hit = r.origin + r.dir * t;

    float fx = hit.x / cfg::CELL + 4.0f;
    float fy = hit.z / cfg::CELL + 4.5f;

    int ix = (int)std::floor(fx + 0.5f);
    int iy = (int)std::floor(fy + 0.5f);

    Pos p{ix, iy};
    if (!xiangqi::inBounds(p)) return false;

    outPos = p;
    return true;
}

static float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

static void glfwErrorCallback(int error, const char* description) {
    util::logError(std::string("GLFW error ") + std::to_string(error) + ": " + (description ? description : ""));
}

static void framebufferSizeCallback(GLFWwindow* window, int w, int h) {
    if (w <= 0 || h <= 0) return;
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;
    app->w = w;
    app->h = h;
    app->renderer.resize(w, h);
}

static void cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (app->input.rmbDown) {
        double dx = x - app->input.lastX;
        double dy = y - app->input.lastY;

        app->cam.yawDeg += (float)dx * 0.25f;
        app->cam.pitchDeg += (float)dy * 0.25f;
        app->cam.pitchDeg = clampf(app->cam.pitchDeg, 15.0f, 85.0f);

        app->input.lastX = x;
        app->input.lastY = y;
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            app->input.rmbDown = true;
            glfwGetCursorPos(window, &app->input.lastX, &app->input.lastY);
        } else if (action == GLFW_RELEASE) {
            app->input.rmbDown = false;
        }
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx = 0.0, my = 0.0;
        glfwGetCursorPos(window, &mx, &my);

        Pos p;
        if (pickBoardPos(app->cam, mx, my, app->w, app->h, p)) {
            app->game.clickAt(p);
        }
        return;
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    app->cam.distance -= (float)yoffset * 0.8f;
    app->cam.distance = clampf(app->cam.distance, 6.0f, 30.0f);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (key == GLFW_KEY_R) {
            app->game.reset();
        }
    }
}

int main() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        util::logError("Failed to init GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    App app;
    app.cam.target = glm::vec3(0.0f, 0.0f, 0.0f);
    app.cam.yawDeg = 45.0f;
    app.cam.pitchDeg = 55.0f;
    app.cam.distance = 16.0f;

    app.window = glfwCreateWindow(app.w, app.h, "Xiangqi3D (OpenGL)", nullptr, nullptr);
    if (!app.window) {
        util::logError("Failed to create window");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(app.window);
    glfwSwapInterval(1);
    

    if (!gladLoadGL()) {
        util::logError("Failed to load OpenGL via glad");
        glfwDestroyWindow(app.window);
        glfwTerminate();
        return 1;
    }

    glfwSetWindowUserPointer(app.window, &app);
    glfwSetFramebufferSizeCallback(app.window, framebufferSizeCallback);
    glfwSetCursorPosCallback(app.window, cursorPosCallback);
    glfwSetMouseButtonCallback(app.window, mouseButtonCallback);
    glfwSetScrollCallback(app.window, scrollCallback);
    glfwSetKeyCallback(app.window, keyCallback);

    util::logInfo(std::string("OpenGL: ") + (const char*)glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initial size (some platforms won't call framebuffer callback immediately)
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(app.window, &fbw, &fbh);
    app.w = fbw > 0 ? fbw : app.w;
    app.h = fbh > 0 ? fbh : app.h;

    if (!app.renderer.init(app.w, app.h)) {
        util::logError("Renderer init failed");
        glfwDestroyWindow(app.window);
        glfwTerminate();
        return 1;
    }

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(app.window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        app.game.update(dt);

        updateWindowTitle(app.window, app.game);
        app.renderer.draw(app.cam, app.game);

        glfwSwapBuffers(app.window);
        glfwPollEvents();
    }

    glfwDestroyWindow(app.window);
    glfwTerminate();
    return 0;
}
