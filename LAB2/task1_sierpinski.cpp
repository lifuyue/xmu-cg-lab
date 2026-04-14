#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Author: 李富悦 22920242203267
// Task 1: 绘制 Sierpinski 镂垫，并逐步叠加颜色、旋转和缩放动画效果。

namespace {

// 基础数据结构与全局状态：维护窗口尺寸、递归层级和动画参数。
struct Vec2 {
    float x;
    float y;
};

int gWindowWidth = 900;
int gWindowHeight = 900;
int gDepth = 6;
int gMode = 3;
bool gPaused = false;
float gTimeSeconds = 0.0f;
float gRotationDegrees = 0.0f;
float gScale = 1.0f;
int gLastTickMs = 0;
int gRenderedFrames = 0;

std::string gCapturePath;
int gCaptureAfterFrames = 90;
bool gCaptureDone = false;

// 颜色与截图工具：负责颜色计算，并将当前帧缓冲区导出为 BMP。
float fract(float value) {
    return value - std::floor(value);
}

void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
    float hh = std::fmod(h, 1.0f);
    if (hh < 0.0f) {
        hh += 1.0f;
    }

    const float scaled = hh * 6.0f;
    const int sector = static_cast<int>(std::floor(scaled));
    const float f = scaled - sector;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));

    switch (sector % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

void saveFramebufferAsBmp(const std::string& path) {
    const int width = gWindowWidth;
    const int height = gWindowHeight;
    const int rowStride = (width * 3 + 3) & ~3;
    const int pixelBytes = rowStride * height;
    const int fileSize = 54 + pixelBytes;

    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

    std::vector<std::uint8_t> bmp(pixelBytes, 0);
    for (int y = 0; y < height; ++y) {
        const std::size_t srcRow = static_cast<std::size_t>(y) * width * 3;
        const std::size_t dstRow = static_cast<std::size_t>(y) * rowStride;
        for (int x = 0; x < width; ++x) {
            bmp[dstRow + x * 3 + 0] = rgb[srcRow + x * 3 + 2];
            bmp[dstRow + x * 3 + 1] = rgb[srcRow + x * 3 + 1];
            bmp[dstRow + x * 3 + 2] = rgb[srcRow + x * 3 + 0];
        }
    }

    std::ofstream out(path, std::ios::binary);
    const std::uint8_t fileHeader[14] = {
        'B', 'M',
        static_cast<std::uint8_t>(fileSize),
        static_cast<std::uint8_t>(fileSize >> 8),
        static_cast<std::uint8_t>(fileSize >> 16),
        static_cast<std::uint8_t>(fileSize >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0
    };
    const std::uint8_t infoHeader[40] = {
        40, 0, 0, 0,
        static_cast<std::uint8_t>(width),
        static_cast<std::uint8_t>(width >> 8),
        static_cast<std::uint8_t>(width >> 16),
        static_cast<std::uint8_t>(width >> 24),
        static_cast<std::uint8_t>(height),
        static_cast<std::uint8_t>(height >> 8),
        static_cast<std::uint8_t>(height >> 16),
        static_cast<std::uint8_t>(height >> 24),
        1, 0, 24, 0,
        0, 0, 0, 0,
        static_cast<std::uint8_t>(pixelBytes),
        static_cast<std::uint8_t>(pixelBytes >> 8),
        static_cast<std::uint8_t>(pixelBytes >> 16),
        static_cast<std::uint8_t>(pixelBytes >> 24),
        19, 11, 0, 0,
        19, 11, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    out.write(reinterpret_cast<const char*>(fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(infoHeader), sizeof(infoHeader));
    out.write(reinterpret_cast<const char*>(bmp.data()), static_cast<std::streamsize>(bmp.size()));
}

// 分形几何与着色：递归生成 Sierpinski 镂垫，并为叶子三角形赋予渐变颜色。
void applyTriangleColor(const Vec2& a, const Vec2& b, const Vec2& c) {
    const float cx = (a.x + b.x + c.x) / 3.0f;
    const float cy = (a.y + b.y + c.y) / 3.0f;
    const float hue = fract(0.18f * (cx + 1.0f) + 0.23f * (cy + 1.2f) + gTimeSeconds * 0.10f);
    const float saturation = 0.65f + 0.15f * std::sin(gTimeSeconds + cx * 2.0f + cy);
    const float value = 0.90f + 0.08f * std::cos(gTimeSeconds * 0.9f + cx - cy);

    float r = 1.0f;
    float g = 1.0f;
    float bValue = 1.0f;
    hsvToRgb(hue, std::clamp(saturation, 0.45f, 0.95f), std::clamp(value, 0.70f, 1.0f), r, g, bValue);
    glColor3f(r, g, bValue);
}

void drawFilledTriangle(const Vec2& a, const Vec2& b, const Vec2& c) {
    applyTriangleColor(a, b, c);
    glBegin(GL_TRIANGLES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glEnd();
}

Vec2 midpoint(const Vec2& lhs, const Vec2& rhs) {
    return {(lhs.x + rhs.x) * 0.5f, (lhs.y + rhs.y) * 0.5f};
}

void drawSierpinski(const Vec2& a, const Vec2& b, const Vec2& c, int depth) {
    if (depth <= 0) {
        drawFilledTriangle(a, b, c);
        return;
    }

    // 每次递归将当前三角形拆成三个子三角形，中心留白部分不再绘制。
    const Vec2 ab = midpoint(a, b);
    const Vec2 bc = midpoint(b, c);
    const Vec2 ca = midpoint(c, a);

    drawSierpinski(a, ab, ca, depth - 1);
    drawSierpinski(ab, b, bc, depth - 1);
    drawSierpinski(ca, bc, c, depth - 1);
}

void drawOutline(const Vec2& a, const Vec2& b, const Vec2& c) {
    glColor3f(0.15f, 0.18f, 0.25f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glEnd();
}

// 动画与渲染：更新时间、旋转和缩放参数，并完成每一帧的绘制。
void updateAnimation(float deltaSeconds) {
    if (gPaused) {
        return;
    }

    gTimeSeconds += deltaSeconds;
    if (gMode >= 2) {
        gRotationDegrees += 30.0f * deltaSeconds;
        if (gRotationDegrees >= 360.0f) {
            gRotationDegrees -= 360.0f;
        }
    }

    gScale = (gMode >= 3) ? (0.86f + 0.12f * std::sin(gTimeSeconds * 1.4f)) : 1.0f;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(gScale, gScale, 1.0f);
    glRotatef((gMode >= 2) ? gRotationDegrees : 0.0f, 0.0f, 0.0f, 1.0f);

    const Vec2 a{0.0f, 0.92f};
    const Vec2 b{-0.92f, -0.72f};
    const Vec2 c{0.92f, -0.72f};

    drawSierpinski(a, b, c, gDepth);
    drawOutline(a, b, c);

    glutSwapBuffers();
    ++gRenderedFrames;

    if (!gCaptureDone && !gCapturePath.empty() && gRenderedFrames >= gCaptureAfterFrames) {
        saveFramebufferAsBmp(gCapturePath);
        gCaptureDone = true;
        glutLeaveMainLoop();
    }
}

// 窗口与输入回调：处理窗口缩放、空闲刷新和键盘交互。
void reshape(int width, int height) {
    gWindowWidth = std::max(width, 1);
    gWindowHeight = std::max(height, 1);

    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const float aspect = static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight);
    if (aspect >= 1.0f) {
        glOrtho(-1.15f * aspect, 1.15f * aspect, -1.15f, 1.15f, -1.0f, 1.0f);
    } else {
        glOrtho(-1.15f, 1.15f, -1.15f / aspect, 1.15f / aspect, -1.0f, 1.0f);
    }
}

void idle() {
    const int currentTick = glutGet(GLUT_ELAPSED_TIME);
    if (gLastTickMs == 0) {
        gLastTickMs = currentTick;
    }

    const float deltaSeconds = static_cast<float>(currentTick - gLastTickMs) / 1000.0f;
    gLastTickMs = currentTick;

    updateAnimation(deltaSeconds);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int /*x*/, int /*y*/) {
    switch (key) {
        case 27:
            glutLeaveMainLoop();
            break;
        case '1':
            gMode = 1;
            gScale = 1.0f;
            break;
        case '2':
            gMode = 2;
            break;
        case '3':
            gMode = 3;
            break;
        case '+':
        case '=':
            gDepth = std::min(gDepth + 1, 8);
            break;
        case '-':
        case '_':
            gDepth = std::max(gDepth - 1, 1);
            break;
        case ' ':
            gPaused = !gPaused;
            break;
        default:
            break;
    }
}

void parseArguments(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--capture" && i + 1 < argc) {
            gCapturePath = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            gCaptureAfterFrames = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--mode" && i + 1 < argc) {
            gMode = std::clamp(std::stoi(argv[++i]), 1, 3);
        } else if (arg == "--depth" && i + 1 < argc) {
            gDepth = std::clamp(std::stoi(argv[++i]), 1, 8);
        }
    }
}

}  // namespace

// 程序入口：初始化 GLUT，注册回调并进入主循环。
int main(int argc, char** argv) {
    parseArguments(argc, argv);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Task1 - Sierpinski Gasket");

    glClearColor(0.97f, 0.97f, 0.99f, 1.0f);
    glShadeModel(GL_FLAT);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
