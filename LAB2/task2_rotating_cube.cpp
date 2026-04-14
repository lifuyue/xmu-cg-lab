#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Author: 李富悦 22920242203267
// Task 2: 实现旋转立方体、深度测试对比，以及可交互的三维相机观察。

namespace {

constexpr float kPi = 3.14159265358979323846f;

// 三维向量工具：为相机运动、朝向与投影计算提供基础运算。
struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 operator+(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vec3 operator-(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 operator*(const Vec3& v, float scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}

float dot(const Vec3& lhs, const Vec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

Vec3 normalize(const Vec3& v) {
    const float length = std::sqrt(dot(v, v));
    if (length <= 1e-6f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v.x / length, v.y / length, v.z / length};
}

// 全局状态：记录窗口、相机、键盘输入以及立方体动画参数。
int gWindowWidth = 1280;
int gWindowHeight = 820;
int gLastTickMs = 0;
int gRenderedFrames = 0;
bool gCaptureDone = false;

Vec3 gCameraPos{0.0f, 0.8f, 7.2f};
float gYawDegrees = -90.0f;
float gPitchDegrees = -10.0f;
bool gMoveLocked = false;
bool gDepthEnabled = true;
bool gUsePerspective = true;
bool gKeys[256] = {};
bool gFirstMouse = true;
int gLastMouseX = 0;
int gLastMouseY = 0;

float gCubeAngleY = 0.0f;
float gCubeAngleX = 0.0f;
float gMoveSpeed = 3.6f;
float gMouseSensitivity = 0.15f;

std::string gCapturePath;
int gCaptureAfterFrames = 100;

// 截图导出：把当前颜色缓冲区保存为 BMP，便于留存实验结果。
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

// 相机与投影：根据 yaw/pitch 求观察方向，并在透视/正交投影间切换。
Vec3 cameraFront() {
    const float yaw = gYawDegrees * kPi / 180.0f;
    const float pitch = gPitchDegrees * kPi / 180.0f;
    return normalize({
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    });
}

Vec3 cameraRight() {
    return normalize(cross(cameraFront(), {0.0f, 1.0f, 0.0f}));
}

void updateProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight);
    if (gUsePerspective) {
        gluPerspective(60.0, aspect, 0.1, 100.0);
    } else {
        const float span = 5.0f;
        if (aspect >= 1.0f) {
            glOrtho(-span * aspect, span * aspect, -span, span, -100.0f, 100.0f);
        } else {
            glOrtho(-span, span, -span / aspect, span / aspect, -100.0f, 100.0f);
        }
    }
    glMatrixMode(GL_MODELVIEW);
}

// 场景绘制：统一组织坐标轴、参考网格和旋转立方体的绘制流程。
void drawAxis(float length) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.25f, 0.25f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(length, 0.0f, 0.0f);
    glColor3f(0.25f, 1.0f, 0.35f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, length, 0.0f);
    glColor3f(0.25f, 0.55f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, length);
    glEnd();
}

void drawReferenceGrid(float halfExtent, float step) {
    glLineWidth(1.0f);
    glColor3f(0.22f, 0.26f, 0.32f);
    glBegin(GL_LINES);
    for (float x = -halfExtent; x <= halfExtent + 0.001f; x += step) {
        glVertex3f(x, -1.5f, -halfExtent);
        glVertex3f(x, -1.5f, halfExtent);
    }
    for (float z = -halfExtent; z <= halfExtent + 0.001f; z += step) {
        glVertex3f(-halfExtent, -1.5f, z);
        glVertex3f(halfExtent, -1.5f, z);
    }
    glEnd();
}

void drawCube() {
    glPushMatrix();
    glRotatef(gCubeAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(gCubeAngleX, 1.0f, 0.0f, 0.0f);

    // 先绘制六个彩色面，再叠加黑色线框，便于观察旋转姿态和遮挡关系。
    glBegin(GL_QUADS);
    glColor3f(0.92f, 0.29f, 0.25f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);

    glColor3f(0.20f, 0.72f, 0.42f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    glColor3f(0.25f, 0.48f, 0.96f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);

    glColor3f(0.98f, 0.72f, 0.16f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);

    glColor3f(0.80f, 0.36f, 0.92f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    glColor3f(0.30f, 0.86f, 0.90f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();

    glLineWidth(2.0f);
    glColor3f(0.08f, 0.08f, 0.10f);
    glBegin(GL_LINES);
    const float s = 1.0f;
    const Vec3 pts[] = {
        {-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
        {-s, -s, s}, {s, -s, s}, {s, s, s}, {-s, s, s}
    };
    const int edges[][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };
    for (const auto& edge : edges) {
        glVertex3f(pts[edge[0]].x, pts[edge[0]].y, pts[edge[0]].z);
        glVertex3f(pts[edge[1]].x, pts[edge[1]].y, pts[edge[1]].z);
    }
    glEnd();

    glPopMatrix();
}

// 文字覆盖层：在屏幕空间显示交互说明和当前状态。
void drawText2D(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char ch : text) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ch);
    }
}

void drawOverlay() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glColor3f(0.95f, 0.95f, 0.98f);
    drawText2D(20.0f, static_cast<float>(gWindowHeight - 28), "WASD move, Q/E up-down, mouse look, L lock, Z toggle depth, P projection, R reset");
    drawText2D(20.0f, static_cast<float>(gWindowHeight - 50),
               std::string("Depth: ") + (gDepthEnabled ? "ON" : "OFF") +
               "   Projection: " + (gUsePerspective ? "Perspective" : "Ortho") +
               "   Camera lock: " + (gMoveLocked ? "ON" : "OFF"));
    if (gDepthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 主渲染流程：设置深度测试状态、装载相机并绘制整个三维场景。
void display() {
    if (gDepthEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    const Vec3 front = cameraFront();
    const Vec3 target = gCameraPos + front;
    gluLookAt(gCameraPos.x, gCameraPos.y, gCameraPos.z,
              target.x, target.y, target.z,
              0.0f, 1.0f, 0.0f);

    drawReferenceGrid(8.0f, 1.0f);
    drawAxis(2.5f);
    drawCube();
    drawOverlay();

    glutSwapBuffers();
    ++gRenderedFrames;

    if (!gCaptureDone && !gCapturePath.empty() && gRenderedFrames >= gCaptureAfterFrames) {
        saveFramebufferAsBmp(gCapturePath);
        gCaptureDone = true;
        glutLeaveMainLoop();
    }
}

// 窗口回调：尺寸变化时同步更新视口和投影矩阵。
void reshape(int width, int height) {
    gWindowWidth = std::max(width, 1);
    gWindowHeight = std::max(height, 1);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    updateProjection();
}

// 输入与相机控制：处理键盘、鼠标和逐帧移动逻辑。
void resetCamera() {
    gCameraPos = {0.0f, 0.8f, 7.2f};
    gYawDegrees = -90.0f;
    gPitchDegrees = -10.0f;
    gFirstMouse = true;
}

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) {
    const unsigned char lowered = static_cast<unsigned char>(std::tolower(key));
    gKeys[lowered] = true;

    switch (lowered) {
        case 27:
            glutLeaveMainLoop();
            break;
        case 'l':
            gMoveLocked = !gMoveLocked;
            gFirstMouse = true;
            break;
        case 'z':
            gDepthEnabled = !gDepthEnabled;
            break;
        case 'p':
            gUsePerspective = !gUsePerspective;
            updateProjection();
            break;
        case 'r':
            resetCamera();
            break;
        default:
            break;
    }
}

void keyboardUp(unsigned char key, int /*x*/, int /*y*/) {
    const unsigned char lowered = static_cast<unsigned char>(std::tolower(key));
    gKeys[lowered] = false;
}

void updateMovement(float deltaSeconds) {
    gCubeAngleY += 40.0f * deltaSeconds;
    gCubeAngleX += 18.0f * deltaSeconds;

    if (gMoveLocked) {
        return;
    }

    const Vec3 front = cameraFront();
    // 平移时忽略俯仰带来的 y 分量，只在水平面做前后移动。
    Vec3 flatFront = normalize({front.x, 0.0f, front.z});
    if (dot(flatFront, flatFront) <= 1e-6f) {
        flatFront = {0.0f, 0.0f, -1.0f};
    }
    const Vec3 right = cameraRight();
    const float distance = gMoveSpeed * deltaSeconds;

    if (gKeys['w']) {
        gCameraPos = gCameraPos + flatFront * distance;
    }
    if (gKeys['s']) {
        gCameraPos = gCameraPos - flatFront * distance;
    }
    if (gKeys['a']) {
        gCameraPos = gCameraPos - right * distance;
    }
    if (gKeys['d']) {
        gCameraPos = gCameraPos + right * distance;
    }
    if (gKeys['q']) {
        gCameraPos.y += distance;
    }
    if (gKeys['e']) {
        gCameraPos.y -= distance;
    }
}

void idle() {
    const int currentTick = glutGet(GLUT_ELAPSED_TIME);
    if (gLastTickMs == 0) {
        gLastTickMs = currentTick;
    }

    const float deltaSeconds = static_cast<float>(currentTick - gLastTickMs) / 1000.0f;
    gLastTickMs = currentTick;

    updateMovement(deltaSeconds);
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (gMoveLocked) {
        return;
    }

    if (gFirstMouse) {
        gLastMouseX = x;
        gLastMouseY = y;
        gFirstMouse = false;
        return;
    }

    const int dx = x - gLastMouseX;
    const int dy = y - gLastMouseY;
    gLastMouseX = x;
    gLastMouseY = y;

    gYawDegrees += static_cast<float>(dx) * gMouseSensitivity;
    gPitchDegrees -= static_cast<float>(dy) * gMouseSensitivity;
    gPitchDegrees = std::clamp(gPitchDegrees, -89.0f, 89.0f);
}

// 参数解析：为自动截图或对比不同投影/深度状态提供命令行开关。
void parseArguments(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--capture" && i + 1 < argc) {
            gCapturePath = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            gCaptureAfterFrames = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--ortho") {
            gUsePerspective = false;
        } else if (arg == "--no-depth") {
            gDepthEnabled = false;
        }
    }
}

}  // namespace

// 程序入口：初始化 OpenGL/GLUT，注册回调并进入事件循环。
int main(int argc, char** argv) {
    parseArguments(argc, argv);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Task2 - Rotating Cube and Camera");

    glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}
