#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Author: 李富悦 22920242203267
// Task 3: 使用基本图元构造线框球体，并结合交互式相机观察其旋转效果。

namespace {

constexpr float kPi = 3.14159265358979323846f;

// 三维向量工具：负责相机方向、移动方向以及旋转轴相关计算。
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

// 全局状态：记录窗口、相机输入以及球体旋转参数。
int gWindowWidth = 1280;
int gWindowHeight = 820;
int gLastTickMs = 0;
int gRenderedFrames = 0;
bool gCaptureDone = false;

Vec3 gCameraPos{0.0f, 0.5f, 6.5f};
float gYawDegrees = -90.0f;
float gPitchDegrees = -8.0f;
bool gMoveLocked = false;
bool gKeys[256] = {};
bool gFirstMouse = true;
int gLastMouseX = 0;
int gLastMouseY = 0;

float gMoveSpeed = 3.2f;
float gMouseSensitivity = 0.15f;
float gSpinY = 0.0f;
float gSpinX = 0.0f;

std::string gCapturePath;
int gCaptureAfterFrames = 100;

// 截图导出：自动保存运行结果，便于整理实验记录。
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

// 相机与投影：根据鼠标输入控制朝向，并在窗口缩放后更新透视投影。
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
    gluPerspective(58.0, aspect, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// 场景绘制：包括坐标轴和由经纬线离散构成的线框球体。
void drawAxis(float length) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.35f, 0.35f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(length, 0.0f, 0.0f);
    glColor3f(0.35f, 1.0f, 0.45f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, length, 0.0f);
    glColor3f(0.35f, 0.55f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, length);
    glEnd();
}

void drawWireSphere(float radius, int slices, int stacks) {
    glLineWidth(1.7f);
    glColor3f(0.76f, 0.88f, 1.0f);

    // 第一层循环固定纬度，绘制一圈圈水平线。
    for (int i = 1; i < stacks; ++i) {
        const float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks);
        const float y = radius * std::cos(phi);
        const float ringRadius = radius * std::sin(phi);

        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < slices; ++j) {
            const float theta = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(slices);
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    // 第二层循环固定经度，绘制从北极到南极的竖向线。
    for (int j = 0; j < slices; ++j) {
        const float theta = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(slices);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= stacks; ++i) {
            const float phi = kPi * static_cast<float>(i) / static_cast<float>(stacks);
            const float x = radius * std::sin(phi) * std::cos(theta);
            const float y = radius * std::cos(phi);
            const float z = radius * std::sin(phi) * std::sin(theta);
            glVertex3f(x, y, z);
        }
        glEnd();
    }
}

// 屏幕文字覆盖层：显示交互说明，避免和三维场景共用投影矩阵。
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
    drawText2D(20.0f, static_cast<float>(gWindowHeight - 28), "WASD move, Q/E up-down, mouse look, L lock, R reset");
    drawText2D(20.0f, static_cast<float>(gWindowHeight - 50), "Wireframe sphere built from latitude and longitude line strips");
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 主渲染流程：装载相机后绘制旋转中的线框球。
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    const Vec3 front = cameraFront();
    const Vec3 target = gCameraPos + front;
    gluLookAt(gCameraPos.x, gCameraPos.y, gCameraPos.z,
              target.x, target.y, target.z,
              0.0f, 1.0f, 0.0f);

    drawAxis(2.5f);

    glPushMatrix();
    glRotatef(gSpinY, 0.0f, 1.0f, 0.0f);
    glRotatef(gSpinX, 1.0f, 0.0f, 0.0f);
    drawWireSphere(1.8f, 28, 18);
    glPopMatrix();

    drawOverlay();
    glutSwapBuffers();
    ++gRenderedFrames;

    if (!gCaptureDone && !gCapturePath.empty() && gRenderedFrames >= gCaptureAfterFrames) {
        saveFramebufferAsBmp(gCapturePath);
        gCaptureDone = true;
        glutLeaveMainLoop();
    }
}

// 窗口回调：根据窗口尺寸变化重新设置视口和透视投影。
void reshape(int width, int height) {
    gWindowWidth = std::max(width, 1);
    gWindowHeight = std::max(height, 1);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    updateProjection();
}

// 输入与动画：处理第一人称相机控制以及球体自转。
void resetCamera() {
    gCameraPos = {0.0f, 0.5f, 6.5f};
    gYawDegrees = -90.0f;
    gPitchDegrees = -8.0f;
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
    gSpinY += 30.0f * deltaSeconds;
    gSpinX += 14.0f * deltaSeconds;

    if (gMoveLocked) {
        return;
    }

    const Vec3 front = cameraFront();
    // 保持前后移动沿水平面进行，避免抬头后直接向上飘移。
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

// 参数解析：支持自动截图模式，便于批量生成运行结果。
void parseArguments(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--capture" && i + 1 < argc) {
            gCapturePath = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            gCaptureAfterFrames = std::max(1, std::stoi(argv[++i]));
        }
    }
}

}  // namespace

// 程序入口：初始化窗口与深度测试，注册回调后进入主循环。
int main(int argc, char** argv) {
    parseArguments(argc, argv);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Task3 - Wireframe Sphere");

    glClearColor(0.05f, 0.07f, 0.11f, 1.0f);
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
