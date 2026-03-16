#if defined(__has_include)
#if __has_include(<GL/freeglut.h>)
#include <GL/freeglut.h>
#elif __has_include(<GLUT/glut.h>)
#include <GLUT/glut.h>
#elif __has_include(<GL/glut.h>)
#include <GL/glut.h>
#else
#error "No GLUT header found."
#endif
#else
#include <GL/freeglut.h>
#endif

#include <cstdlib>
#include <cmath>

namespace
{
    enum ShapeType
    {
        SHAPE_SQUARE,
        SHAPE_CIRCLE
    };

    enum ColorMode
    {
        COLOR_SOLID,
        COLOR_PER_VERTEX
    };

    constexpr float MOVE_STEP = 0.1f;
    constexpr float MAX_OFFSET = 0.5f;
    constexpr int CIRCLE_SEGMENTS = 40;

    ShapeType currentShape = SHAPE_SQUARE;
    ColorMode currentColorMode = COLOR_PER_VERTEX;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    void resetState()
    {
        currentShape = SHAPE_SQUARE;
        currentColorMode = COLOR_PER_VERTEX;
        offsetX = 0.0f;
        offsetY = 0.0f;
    }

    float clampOffset(float value)
    {
        if (value > MAX_OFFSET)
        {
            return MAX_OFFSET;
        }

        if (value < -MAX_OFFSET)
        {
            return -MAX_OFFSET;
        }

        return value;
    }

    void drawSquare()
    {
        glBegin(GL_POLYGON);
        if (currentColorMode == COLOR_PER_VERTEX)
        {
            glColor3f(1.0f, 0.2f, 0.2f);
            glVertex2f(-0.4f, -0.4f);

            glColor3f(0.2f, 1.0f, 0.2f);
            glVertex2f(-0.4f, 0.4f);

            glColor3f(0.2f, 0.4f, 1.0f);
            glVertex2f(0.4f, 0.4f);

            glColor3f(1.0f, 0.9f, 0.2f);
            glVertex2f(0.4f, -0.4f);
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(-0.4f, -0.4f);
            glVertex2f(-0.4f, 0.4f);
            glVertex2f(0.4f, 0.4f);
            glVertex2f(0.4f, -0.4f);
        }
        glEnd();
    }

    void drawCircle(int segments)
    {
        const float radius = 0.45f;
        const float pi = 3.14159265358979323846f;

        glBegin(GL_TRIANGLE_FAN);
        if (currentColorMode == COLOR_PER_VERTEX)
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);

            for (int i = 0; i <= segments; ++i)
            {
                const float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
                const float x = radius * std::cos(angle);
                const float y = radius * std::sin(angle);

                glColor3f((std::cos(angle) + 1.0f) * 0.5f,
                          (std::sin(angle) + 1.0f) * 0.5f,
                          1.0f - static_cast<float>(i) / static_cast<float>(segments));
                glVertex2f(x, y);
            }
        }
        else
        {
            glColor3f(0.8f, 0.8f, 1.0f);
            glVertex2f(0.0f, 0.0f);

            for (int i = 0; i <= segments; ++i)
            {
                const float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
                const float x = radius * std::cos(angle);
                const float y = radius * std::sin(angle);
                glVertex2f(x, y);
            }
        }
        glEnd();
    }
}

void display(void)
{
    /* 清除窗口 */
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glTranslatef(offsetX, offsetY, 0.0f);

    if (currentShape == SHAPE_SQUARE)
    {
        drawSquare();
    }
    else
    {
        drawCircle(CIRCLE_SEGMENTS);
    }

    glPopMatrix();

    /* 双缓冲交换显示内容 */
    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
        case '1':
            currentShape = SHAPE_SQUARE;
            break;
        case '2':
            currentShape = SHAPE_CIRCLE;
            break;
        case 'c':
        case 'C':
            currentColorMode = (currentColorMode == COLOR_SOLID) ? COLOR_PER_VERTEX : COLOR_SOLID;
            break;
        case 'r':
        case 'R':
            resetState();
            break;
        case 27:
            std::exit(0);
            return;
        default:
            return;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    switch (key)
    {
        case GLUT_KEY_LEFT:
            offsetX = clampOffset(offsetX - MOVE_STEP);
            break;
        case GLUT_KEY_RIGHT:
            offsetX = clampOffset(offsetX + MOVE_STEP);
            break;
        case GLUT_KEY_UP:
            offsetY = clampOffset(offsetY + MOVE_STEP);
            break;
        case GLUT_KEY_DOWN:
            offsetY = clampOffset(offsetY - MOVE_STEP);
            break;
        default:
            return;
    }

    glutPostRedisplay();
}

void init()
{
    /* 设置清屏颜色为黑色 */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* 设置二维正交投影，便于观察对象移动 */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
}

int main(int argc, char** argv)
{
    /* 初始化 GLUT 并打开窗口 */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    /* 设置窗口大小和初始位置 */
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(0, 0);

    /* 创建窗口，标题为 "interactive shapes" */
    glutCreateWindow("interactive shapes");

    /* 注册回调函数 */
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    /* 调用我们自己的初始化函数 */
    init();

    /* 进入 GLUT 事件处理循环 */
    glutMainLoop();

    return 0;
}
