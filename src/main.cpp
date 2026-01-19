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

struct PromptLayout {
    UiRect panel;
    UiRect restart;
    UiRect exit;
};

enum class AppMode {
    Menu,
    Loading,
    Playing,
};

struct App {
    GLFWwindow* window = nullptr;
    int w = 1280;
    int h = 720;

    OrbitCamera cam;
    Renderer renderer;
    XiangqiGame game;
    AppMode mode = AppMode::Menu;
    bool helpShown = false;

    InputState input;
};

static void updateWindowTitle(GLFWwindow* window, const XiangqiGame& game, AppMode mode) {
    static std::string last;
    std::string title;
    if (mode == AppMode::Menu) {
        title = "Xiangqi3D (OpenGL) - Menu";
    } else if (mode == AppMode::Loading) {
        title = "Xiangqi3D (OpenGL) - Loading";
    } else {
        title = std::string("Xiangqi3D (OpenGL) - ") + game.windowTitleCN();
    }
    if (title != last) {
        glfwSetWindowTitle(window, title.c_str());
        last = std::move(title);
    }
}

static UiRect makeButton(float cx, float cy, float w, float h) {
    UiRect r;
    r.x = cx - w * 0.5f;
    r.y = cy - h * 0.5f;
    r.w = w;
    r.h = h;
    return r;
}

static MenuLayout makeMenuLayout(int w, int h) {
    MenuLayout layout;
    float bw = 260.0f;
    float bh = 70.0f;
    float cx = w * 0.5f;
    float cy = h * 0.5f - 120.0f;
    layout.start = makeButton(cx, cy + 60.0f, bw, bh);
    layout.exit = makeButton(cx, cy - 60.0f, bw, bh);
    return layout;
}

static PromptLayout makePromptLayout(int w, int h) {
    PromptLayout layout;
    float panelW = 460.0f;
    float panelH = 220.0f;
    layout.panel = UiRect{((float)w - panelW) * 0.5f, ((float)h - panelH) * 0.5f, panelW, panelH};

    float bw = 165.0f;
    float bh = 54.0f;
    layout.restart = UiRect{layout.panel.x + layout.panel.w * 0.5f - bw - 18.0f, layout.panel.y + 32.0f, bw, bh};
    layout.exit = UiRect{layout.panel.x + layout.panel.w * 0.5f + 18.0f, layout.panel.y + 32.0f, bw, bh};
    return layout;
}

static bool pointInRect(float x, float y, const UiRect& r) {
    return x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
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
    if (app->mode != AppMode::Playing) return;

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

        if (app->mode == AppMode::Menu) {
            MenuLayout layout = makeMenuLayout(app->w, app->h);
            float sx = (float)mx;
            float sy = (float)app->h - (float)my;
            if (pointInRect(sx, sy, layout.start)) {
                if (app->renderer.isPreloadReady()) {
                    app->game.reset();
                    app->mode = AppMode::Playing;
                    if (!app->helpShown) {
                        app->game.startHelp(6.0f);
                        app->helpShown = true;
                    }
                } else {
                    app->mode = AppMode::Loading;
                }
            } else if (pointInRect(sx, sy, layout.exit)) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            return;
        }

        if (app->mode == AppMode::Loading) {
            return;
        }

        if (app->mode == AppMode::Playing && app->game.resultPromptActive()) {
            PromptLayout layout = makePromptLayout(app->w, app->h);
            float sx = (float)mx;
            float sy = (float)app->h - (float)my;
            if (pointInRect(sx, sy, layout.restart)) {
                app->game.reset();
            } else if (pointInRect(sx, sy, layout.exit)) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            return;
        }

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
    if (app->mode != AppMode::Playing) return;

    app->cam.distance -= (float)yoffset * 0.8f;
    app->cam.distance = clampf(app->cam.distance, 6.0f, 30.0f);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    auto* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (action == GLFW_PRESS) {
        if (app->mode == AppMode::Menu) {
            if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (app->renderer.isPreloadReady()) {
                    app->game.reset();
                    app->mode = AppMode::Playing;
                    if (!app->helpShown) {
                        app->game.startHelp(6.0f);
                        app->helpShown = true;
                    }
                } else {
                    app->mode = AppMode::Loading;
                }
                return;
            }
            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                return;
            }
        }
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
    app.cam.yawDeg = -90.0f;
    app.cam.pitchDeg = 52.0f;
    app.cam.distance = 15.0f;

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
    app.renderer.beginPreload();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(app.window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        if (app.mode == AppMode::Playing) {
            app.game.update(dt);
        }

        updateWindowTitle(app.window, app.game, app.mode);
        if (app.mode == AppMode::Menu) {
            MenuLayout layout = makeMenuLayout(app.w, app.h);
            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(app.window, &mx, &my);
            float sx = (float)mx;
            float sy = (float)app.h - (float)my;
            bool hoverStart = pointInRect(sx, sy, layout.start);
            bool hoverExit = pointInRect(sx, sy, layout.exit);
            app.renderer.drawMenu(layout, hoverStart, hoverExit, true);
        } else if (app.mode == AppMode::Loading) {
            app.renderer.drawLoading(u8"\u6b63\u5728\u52a0\u8f7d\u8d44\u6e90\u002e\u002e\u002e\u002e\u002e\u002e");
        } else {
            app.renderer.draw(app.cam, app.game);
        }

        glfwSwapBuffers(app.window);
        glfwPollEvents();

        if (!app.renderer.isPreloadReady()) {
            app.renderer.preloadStep(1);
        } else if (app.mode == AppMode::Loading) {
            app.game.reset();
            app.mode = AppMode::Playing;
            if (!app.helpShown) {
                app.game.startHelp(6.0f);
                app.helpShown = true;
            }
        }
    }

    glfwDestroyWindow(app.window);
    glfwTerminate();
    return 0;
}
