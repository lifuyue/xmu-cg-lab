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
#include <fstream>
#include <cstdlib>

namespace
{
    struct Color
    {
        float r;
        float g;
        float b;
    };

    struct Ring
    {
        float x;
        float y;
        Color color;
    };

    constexpr int WINDOW_WIDTH = 900;
    constexpr int WINDOW_HEIGHT = 600;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float RING_RADIUS = 0.34f;
    constexpr float RING_THICKNESS = 0.045f;
    // Upper-row adjacent rings should be clearly separated from the center ring.
    constexpr float TOP_ROW_SPACING = 2.90f * RING_RADIUS;
    // Align the visible top of the lower rings with the upper-row center line.
    constexpr float LOWER_ROW_DROP = RING_RADIUS + RING_THICKNESS * 0.5f;
    constexpr float GROUP_SCALE = 1.0f;
    constexpr float GROUP_OFFSET_Y = 0.08f;
    constexpr int ARC_SEGMENTS = 360;
    constexpr float TOP_LEFT_START = 45.0f;
    constexpr float TOP_RIGHT_START = 225.0f;
    constexpr float BOTTOM_LEFT_START = 315.0f;
    constexpr float BOTTOM_RIGHT_START = 135.0f;
    constexpr float HALF_ARC_SPAN = 180.0f;
    constexpr bool EXPORT_DEBUG_FRAME = true;

    constexpr Ring rings[5] = {
        {-TOP_ROW_SPACING, LOWER_ROW_DROP * 0.5f, {0.0f, 0.52f, 0.86f}},
        {0.0f, LOWER_ROW_DROP * 0.5f, {0.0f, 0.0f, 0.0f}},
        {TOP_ROW_SPACING, LOWER_ROW_DROP * 0.5f, {0.93f, 0.16f, 0.32f}},
        {-TOP_ROW_SPACING * 0.5f, -LOWER_ROW_DROP * 0.5f, {1.0f, 0.68f, 0.12f}},
        {TOP_ROW_SPACING * 0.5f, -LOWER_ROW_DROP * 0.5f, {0.0f, 0.69f, 0.31f}},
    };

    float normalizeAngle(float angle)
    {
        while (angle < 0.0f)
        {
            angle += 360.0f;
        }

        while (angle >= 360.0f)
        {
            angle -= 360.0f;
        }

        return angle;
    }

    void drawArc(const Ring& ring, float startDeg, float endDeg, const Color& color, float thickness)
    {
        const float normalizedStart = normalizeAngle(startDeg);
        float normalizedEnd = normalizeAngle(endDeg);

        if (normalizedEnd <= normalizedStart)
        {
            normalizedEnd += 360.0f;
        }

        glColor3f(color.r, color.g, color.b);
        glBegin(GL_TRIANGLE_STRIP);

        const float outerRadius = RING_RADIUS + thickness * 0.5f;
        const float innerRadius = RING_RADIUS - thickness * 0.5f;
        for (int i = 0; i <= ARC_SEGMENTS; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(ARC_SEGMENTS);
            const float angleDeg = normalizedStart + (normalizedEnd - normalizedStart) * t;
            const float angleRad = angleDeg * PI / 180.0f;

            glVertex2f(
                ring.x + outerRadius * std::cos(angleRad),
                ring.y + outerRadius * std::sin(angleRad));
            glVertex2f(
                ring.x + innerRadius * std::cos(angleRad),
                ring.y + innerRadius * std::sin(angleRad));
        }

        glEnd();
    }

    void drawHalfArc(const Ring& ring, float startAngle)
    {
        drawArc(ring, startAngle, startAngle + HALF_ARC_SPAN, ring.color, RING_THICKNESS);
    }

    void init()
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void reshape(int width, int height)
    {
        constexpr float VIEW_HALF_HEIGHT = 1.0f;

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
            glOrtho(-VIEW_HALF_HEIGHT * aspect, VIEW_HALF_HEIGHT * aspect, -VIEW_HALF_HEIGHT, VIEW_HALF_HEIGHT, -1.0, 1.0);
        }
        else
        {
            glOrtho(-VIEW_HALF_HEIGHT, VIEW_HALF_HEIGHT, -VIEW_HALF_HEIGHT / aspect, VIEW_HALF_HEIGHT / aspect, -1.0, 1.0);
        }

        glMatrixMode(GL_MODELVIEW);
    }

    void drawOlympicRings()
    {
        drawHalfArc(rings[0], TOP_LEFT_START);
        drawHalfArc(rings[1], TOP_LEFT_START);
        drawHalfArc(rings[3], BOTTOM_LEFT_START);
        drawHalfArc(rings[0], TOP_RIGHT_START);
        drawHalfArc(rings[3], BOTTOM_RIGHT_START);
        drawHalfArc(rings[2], TOP_LEFT_START);
        drawHalfArc(rings[4], BOTTOM_LEFT_START);
        drawHalfArc(rings[1], TOP_RIGHT_START);
        drawHalfArc(rings[4], BOTTOM_RIGHT_START);
        drawHalfArc(rings[2], TOP_RIGHT_START);
    }

    void exportDebugFrame()
    {
        if (!EXPORT_DEBUG_FRAME)
        {
            return;
        }

        static bool exported = false;
        if (exported)
        {
            return;
        }

        const int width = glutGet(GLUT_WINDOW_WIDTH);
        const int height = glutGet(GLUT_WINDOW_HEIGHT);
        if (width <= 0 || height <= 0)
        {
            return;
        }

        unsigned char* pixels = new unsigned char[static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3];
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        std::ofstream output("/tmp/lab1-task3.ppm", std::ios::binary);
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (int y = height - 1; y >= 0; --y)
        {
            const unsigned char* row = pixels + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 3;
            output.write(reinterpret_cast<const char*>(row), static_cast<std::streamsize>(width) * 3);
        }

        delete[] pixels;
        exported = true;
    }

    void display()
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, GROUP_OFFSET_Y, 0.0f);
        glScalef(GROUP_SCALE, GROUP_SCALE, 1.0f);

        drawOlympicRings();
        exportDebugFrame();

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
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("LAB1 Task3 - Olympic Rings");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
