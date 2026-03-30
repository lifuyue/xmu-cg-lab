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

#include <cstdlib>

namespace
{
    void init()
    {
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
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

    void display()
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.2f, 0.2f);
        glVertex2f(-0.6f, -0.4f);

        glColor3f(0.2f, 1.0f, 0.3f);
        glVertex2f(0.6f, -0.4f);

        glColor3f(0.2f, 0.4f, 1.0f);
        glVertex2f(0.0f, 0.65f);
        glEnd();

        glutSwapBuffers();
    }

    void keyboard(unsigned char key, int, int)
    {
        if (key == 27)
        {
            std::exit(0);
        }
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("LAB1 Task1 - OpenGL Example");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
