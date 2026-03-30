#if defined(_WIN32)
#define FREEGLUT_STATIC
#endif

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

#include <cmath>
#include <cstdlib>

namespace
{
    constexpr int WINDOW_WIDTH = 700;
    constexpr int WINDOW_HEIGHT = 700;
    constexpr int SECTOR_COUNT = 8;
    constexpr float RADIUS = 0.82f;
    constexpr float PI = 3.14159265358979323846f;
    constexpr int ARC_SEGMENTS_PER_SECTOR = 48;

    float rotationAngle = 0.0f;
    float rotationSpeed = 1.5f;
    bool isAnimating = true;

    const float colors[SECTOR_COUNT][3] = {
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.95f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.1f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 0.5f, 0.0f},
        {0.1f, 0.75f, 0.95f},
        {0.6f, 0.0f, 0.9f},
    };

    void init()
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void reshape(int width, int height)
    {
        if (height == 0)
        {
            height = 1;
        }

        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        if (aspect >= 1.0f)
        {
            glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
        }
        else
        {
            glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
        }

        glMatrixMode(GL_MODELVIEW);
    }

    void drawSector(float startAngle, float endAngle, const float color[3])
    {
        glColor3fv(color);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f, 0.0f);

        for (int i = 0; i <= ARC_SEGMENTS_PER_SECTOR; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(ARC_SEGMENTS_PER_SECTOR);
            const float angle = startAngle + (endAngle - startAngle) * t;
            glVertex2f(RADIUS * std::cos(angle), RADIUS * std::sin(angle));
        }

        glEnd();
    }

    void drawCircle()
    {
        const float angleStep = 2.0f * PI / static_cast<float>(SECTOR_COUNT);
        for (int i = 0; i < SECTOR_COUNT; ++i)
        {
            const float startAngle = static_cast<float>(i) * angleStep;
            const float endAngle = static_cast<float>(i + 1) * angleStep;
            drawSector(startAngle, endAngle, colors[i]);
        }
    }

    void display()
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);

        drawCircle();

        glutSwapBuffers();
    }

    void timer(int)
    {
        if (isAnimating)
        {
            rotationAngle += rotationSpeed;
            if (rotationAngle >= 360.0f)
            {
                rotationAngle -= 360.0f;
            }
        }

        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
    }

    void keyboard(unsigned char key, int, int)
    {
        switch (key)
        {
            case 27:
                std::exit(0);
                break;
            case ' ':
                isAnimating = !isAnimating;
                break;
            case '+':
            case '=':
                rotationSpeed += 0.5f;
                break;
            case '-':
            case '_':
                rotationSpeed -= 0.5f;
                if (rotationSpeed < 0.0f)
                {
                    rotationSpeed = 0.0f;
                }
                break;
            case 'r':
            case 'R':
                rotationAngle = 0.0f;
                rotationSpeed = 1.5f;
                isAnimating = true;
                break;
            default:
                break;
        }

        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(120, 80);
    glutCreateWindow("LAB1 Task2 - Colorful Solid Circle");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
