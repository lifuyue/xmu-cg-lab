#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Author: 李富悦 22920242203267
// Experiment 3: PLY 3D model display, lighting animation, and lit rotating cube.

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
};

struct Face {
    int a;
    int b;
    int c;
};

struct Material {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess;
};

struct Slider {
    std::string label;
    float value;
    float minValue;
    float maxValue;
    int x;
    int y;
    int width;
    int height;
};

int gWindowWidth = 1280;
int gWindowHeight = 860;
int gLastTickMs = 0;
int gMode = 1;
int gActiveSlider = -1;

std::vector<Vertex> gVertices;
std::vector<Face> gFaces;
Vec3 gModelCenter{0.0f, 0.0f, 0.0f};
float gModelScale = 1.0f;
bool gModelLoaded = false;
std::string gModelPath = "lizhenxiout.ply";

float gModelYaw = -18.0f;
float gModelPitch = 5.0f;
float gModelAutoYaw = 0.0f;
float gZoom = 2.7f;
float gLightAngle = 0.0f;
float gCubeAngle = 0.0f;
bool gAutoRotate = true;
bool gUseColoredLight = false;

Material gMaterial{};
std::string gMaterialName = "Brass";
std::vector<Slider> gSliders;

Vec3 operator+(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vec3 operator-(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 operator*(const Vec3& value, float scalar) {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

float clampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

bool fileExists(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    return in.good();
}

std::string directoryOf(const std::string& path) {
    const std::size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    return path.substr(0, pos);
}

std::string findModelPath(int argc, char** argv) {
    std::vector<std::string> candidates;
    if (argc > 1) {
        candidates.push_back(argv[1]);
    }

    const std::string exeDir = (argc > 0) ? directoryOf(argv[0]) : ".";
    candidates.push_back("lizhenxiout.ply");
    candidates.push_back("LAB3/lizhenxiout.ply");
    candidates.push_back(exeDir + "/lizhenxiout.ply");
    candidates.push_back(exeDir + "/LAB3/lizhenxiout.ply");
    candidates.push_back("../LAB3/lizhenxiout.ply");

    for (const std::string& path : candidates) {
        if (fileExists(path)) {
            return path;
        }
    }
    return candidates.front();
}

bool loadPly(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in) {
        std::cerr << "Cannot open PLY file: " << path << std::endl;
        return false;
    }

    std::string line;
    int vertexCount = 0;
    int faceCount = 0;
    bool asciiPly = false;
    while (std::getline(in, line)) {
        if (line.find("format ascii") == 0) {
            asciiPly = true;
        } else if (line.find("element vertex") == 0) {
            std::istringstream ss(line);
            std::string a;
            std::string b;
            ss >> a >> b >> vertexCount;
        } else if (line.find("element face") == 0) {
            std::istringstream ss(line);
            std::string a;
            std::string b;
            ss >> a >> b >> faceCount;
        } else if (line == "end_header") {
            break;
        }
    }

    if (!asciiPly || vertexCount <= 0 || faceCount <= 0) {
        std::cerr << "Unsupported or invalid PLY file." << std::endl;
        return false;
    }

    gVertices.assign(static_cast<std::size_t>(vertexCount), Vertex{});
    Vec3 minPoint{1e30f, 1e30f, 1e30f};
    Vec3 maxPoint{-1e30f, -1e30f, -1e30f};

    for (int i = 0; i < vertexCount; ++i) {
        Vertex v{};
        in >> v.position.x >> v.position.y >> v.position.z
           >> v.normal.x >> v.normal.y >> v.normal.z;
        gVertices[static_cast<std::size_t>(i)] = v;
        minPoint.x = std::min(minPoint.x, v.position.x);
        minPoint.y = std::min(minPoint.y, v.position.y);
        minPoint.z = std::min(minPoint.z, v.position.z);
        maxPoint.x = std::max(maxPoint.x, v.position.x);
        maxPoint.y = std::max(maxPoint.y, v.position.y);
        maxPoint.z = std::max(maxPoint.z, v.position.z);
    }

    gFaces.clear();
    gFaces.reserve(static_cast<std::size_t>(faceCount));
    for (int i = 0; i < faceCount; ++i) {
        int n = 0;
        Face f{};
        in >> n;
        if (n == 3) {
            in >> f.a >> f.b >> f.c;
            gFaces.push_back(f);
        } else {
            std::vector<int> ignored(static_cast<std::size_t>(n));
            for (int j = 0; j < n; ++j) {
                in >> ignored[static_cast<std::size_t>(j)];
            }
        }
    }

    gModelCenter = (minPoint + maxPoint) * 0.5f;
    const Vec3 size = maxPoint - minPoint;
    gModelScale = std::max(size.x, std::max(size.y, size.z));
    if (gModelScale <= 1e-6f) {
        gModelScale = 1.0f;
    }

    std::cout << "Loaded " << path << ": " << gVertices.size()
              << " vertices, " << gFaces.size() << " faces" << std::endl;
    return true;
}

void setMaterial(const Material& material, const std::string& name) {
    gMaterial = material;
    gMaterialName = name;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, gMaterial.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, gMaterial.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, gMaterial.specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, gMaterial.shininess);
}

Material makeMaterial(float ar, float ag, float ab,
                      float dr, float dg, float db,
                      float sr, float sg, float sb,
                      float shininess) {
    Material material{};
    material.ambient[0] = ar;
    material.ambient[1] = ag;
    material.ambient[2] = ab;
    material.ambient[3] = 1.0f;
    material.diffuse[0] = dr;
    material.diffuse[1] = dg;
    material.diffuse[2] = db;
    material.diffuse[3] = 1.0f;
    material.specular[0] = sr;
    material.specular[1] = sg;
    material.specular[2] = sb;
    material.specular[3] = 1.0f;
    material.shininess = shininess;
    return material;
}

void applySliderScaleToMaterial() {
    const float ambientScale = gSliders[0].value;
    const float diffuseScale = gSliders[1].value;
    const float specularScale = gSliders[2].value;
    const float shininess = gSliders[3].value;

    GLfloat ambient[4] = {
        gMaterial.ambient[0] * ambientScale,
        gMaterial.ambient[1] * ambientScale,
        gMaterial.ambient[2] * ambientScale,
        1.0f
    };
    GLfloat diffuse[4] = {
        gMaterial.diffuse[0] * diffuseScale,
        gMaterial.diffuse[1] * diffuseScale,
        gMaterial.diffuse[2] * diffuseScale,
        1.0f
    };
    GLfloat specular[4] = {
        gMaterial.specular[0] * specularScale,
        gMaterial.specular[1] * specularScale,
        gMaterial.specular[2] * specularScale,
        1.0f
    };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void initSliders() {
    gSliders.clear();
    gSliders.push_back({"Ambient", 1.0f, 0.0f, 2.0f, 24, 0, 230, 18});
    gSliders.push_back({"Diffuse", 1.0f, 0.0f, 2.0f, 24, 0, 230, 18});
    gSliders.push_back({"Specular", 1.0f, 0.0f, 2.5f, 24, 0, 230, 18});
    gSliders.push_back({"Shininess", 42.0f, 1.0f, 128.0f, 24, 0, 230, 18});
}

void updateSliderLayout() {
    const int startY = gWindowHeight - 116;
    for (std::size_t i = 0; i < gSliders.size(); ++i) {
        gSliders[i].x = 24;
        gSliders[i].y = startY + static_cast<int>(i) * 24;
        gSliders[i].width = 230;
        gSliders[i].height = 16;
    }
}

void initLighting() {
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    GLfloat globalAmbient[] = {0.18f, 0.18f, 0.18f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glEnable(GL_LIGHT0);
}

void setLightColor() {
    GLfloat whiteDiffuse[] = {0.95f, 0.95f, 0.95f, 1.0f};
    GLfloat whiteSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat coloredDiffuse[] = {0.95f, 0.72f, 0.38f, 1.0f};
    GLfloat coloredSpecular[] = {0.40f, 0.80f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_DIFFUSE, gUseColoredLight ? coloredDiffuse : whiteDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, gUseColoredLight ? coloredSpecular : whiteSpecular);
}

void updateLightPosition() {
    const float radius = 2.9f;
    GLfloat position[] = {
        std::cos(gLightAngle) * radius,
        1.25f,
        std::sin(gLightAngle) * radius,
        1.0f
    };
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

void updateProjection3D() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = static_cast<float>(gWindowWidth) / std::max(1, gWindowHeight);
    gluPerspective(48.0, aspect, 0.05, 80.0);
    glMatrixMode(GL_MODELVIEW);
}

void drawString(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char ch : text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, ch);
    }
}

void drawModel(bool lit) {
    if (!gModelLoaded) {
        return;
    }

    if (lit) {
        glEnable(GL_LIGHTING);
        applySliderScaleToMaterial();
    } else {
        glDisable(GL_LIGHTING);
        glColor3f(0.22f, 0.58f, 0.82f);
    }

    glPushMatrix();
    glTranslatef(0.0f, -0.06f, 0.0f);
    glRotatef(gModelPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(gModelYaw + gModelAutoYaw, 0.0f, 1.0f, 0.0f);
    glScalef(2.25f / gModelScale, 2.25f / gModelScale, 2.25f / gModelScale);
    glTranslatef(-gModelCenter.x, -gModelCenter.y, -gModelCenter.z);

    glBegin(GL_TRIANGLES);
    for (const Face& face : gFaces) {
        const Vertex& a = gVertices[static_cast<std::size_t>(face.a)];
        const Vertex& b = gVertices[static_cast<std::size_t>(face.b)];
        const Vertex& c = gVertices[static_cast<std::size_t>(face.c)];

        glNormal3f(a.normal.x, a.normal.y, a.normal.z);
        glVertex3f(a.position.x, a.position.y, a.position.z);
        glNormal3f(b.normal.x, b.normal.y, b.normal.z);
        glVertex3f(b.position.x, b.position.y, b.position.z);
        glNormal3f(c.normal.x, c.normal.y, c.normal.z);
        glVertex3f(c.position.x, c.position.y, c.position.z);
    }
    glEnd();
    glPopMatrix();
}

void drawCubeFace(const Vec3& normal,
                  const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) {
    glNormal3f(normal.x, normal.y, normal.z);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
    glVertex3f(c.x, c.y, c.z);
    glVertex3f(d.x, d.y, d.z);
}

void drawLitCube() {
    glEnable(GL_LIGHTING);
    applySliderScaleToMaterial();

    glPushMatrix();
    glRotatef(gCubeAngle, 0.5f, 1.0f, 0.1f);
    glScalef(1.35f, 1.35f, 1.35f);

    glBegin(GL_QUADS);
    drawCubeFace({0.0f, 0.0f, 1.0f}, {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1});
    drawCubeFace({0.0f, 0.0f, -1.0f}, {1,-1,-1}, {-1,-1,-1}, {-1,1,-1}, {1,1,-1});
    drawCubeFace({-1.0f, 0.0f, 0.0f}, {-1,-1,-1}, {-1,-1,1}, {-1,1,1}, {-1,1,-1});
    drawCubeFace({1.0f, 0.0f, 0.0f}, {1,-1,1}, {1,-1,-1}, {1,1,-1}, {1,1,1});
    drawCubeFace({0.0f, 1.0f, 0.0f}, {-1,1,1}, {1,1,1}, {1,1,-1}, {-1,1,-1});
    drawCubeFace({0.0f, -1.0f, 0.0f}, {-1,-1,-1}, {1,-1,-1}, {1,-1,1}, {-1,-1,1});
    glEnd();

    glDisable(GL_LIGHTING);
    glColor3f(0.06f, 0.06f, 0.07f);
    glutWireCube(2.01);
    glPopMatrix();
}

void drawGround() {
    glDisable(GL_LIGHTING);
    glColor3f(0.78f, 0.80f, 0.82f);
    glBegin(GL_LINES);
    for (int i = -6; i <= 6; ++i) {
        glVertex3f(static_cast<float>(i) * 0.45f, -1.28f, -2.8f);
        glVertex3f(static_cast<float>(i) * 0.45f, -1.28f, 2.8f);
        glVertex3f(-2.8f, -1.28f, static_cast<float>(i) * 0.45f);
        glVertex3f(2.8f, -1.28f, static_cast<float>(i) * 0.45f);
    }
    glEnd();
}

void drawLightMarker() {
    const float radius = 2.9f;
    const float x = std::cos(gLightAngle) * radius;
    const float z = std::sin(gLightAngle) * radius;
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(x, 1.25f, z);
    glColor3f(gUseColoredLight ? 1.0f : 0.98f, gUseColoredLight ? 0.72f : 0.98f, 0.35f);
    glutSolidSphere(0.08, 16, 12);
    glPopMatrix();
}

void beginOverlay() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, gWindowWidth, gWindowHeight, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

void endOverlay() {
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawSlider(const Slider& slider) {
    const float t = (slider.value - slider.minValue) / (slider.maxValue - slider.minValue);
    const int knobX = slider.x + static_cast<int>(t * slider.width);

    glColor3f(0.18f, 0.20f, 0.22f);
    drawString(static_cast<float>(slider.x), static_cast<float>(slider.y - 4), slider.label);

    glColor3f(0.84f, 0.86f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2i(slider.x, slider.y);
    glVertex2i(slider.x + slider.width, slider.y);
    glVertex2i(slider.x + slider.width, slider.y + slider.height);
    glVertex2i(slider.x, slider.y + slider.height);
    glEnd();

    glColor3f(0.20f, 0.45f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2i(slider.x, slider.y);
    glVertex2i(knobX, slider.y);
    glVertex2i(knobX, slider.y + slider.height);
    glVertex2i(slider.x, slider.y + slider.height);
    glEnd();

    glColor3f(0.10f, 0.10f, 0.11f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(slider.x, slider.y);
    glVertex2i(slider.x + slider.width, slider.y);
    glVertex2i(slider.x + slider.width, slider.y + slider.height);
    glVertex2i(slider.x, slider.y + slider.height);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2i(knobX - 4, slider.y - 3);
    glVertex2i(knobX + 4, slider.y - 3);
    glVertex2i(knobX + 4, slider.y + slider.height + 3);
    glVertex2i(knobX - 4, slider.y + slider.height + 3);
    glEnd();

    std::ostringstream ss;
    ss.precision(2);
    ss << std::fixed << slider.value;
    drawString(static_cast<float>(slider.x + slider.width + 14),
               static_cast<float>(slider.y + 12), ss.str());
}

void drawOverlay() {
    beginOverlay();
    glColor3f(0.05f, 0.06f, 0.07f);
    drawString(18.0f, 22.0f, "Experiment 3 - 3D Model Display");
    drawString(18.0f, 42.0f, "1 Solid PLY   2 Lit PLY + rotating light   3 Lit rotating cube");
    drawString(18.0f, 62.0f, "b Brass   n Red plastic   m Shiny white   o White light   p Colored light");
    drawString(18.0f, 82.0f, "Mouse drag sliders or use +/- zoom, arrows rotate, space pause animation");

    std::ostringstream state;
    state << "Mode=" << gMode
          << "  Material=" << gMaterialName
          << "  Light=" << (gUseColoredLight ? "Colored" : "White")
          << "  PLY vertices=" << gVertices.size()
          << " faces=" << gFaces.size();
    drawString(18.0f, 104.0f, state.str());

    for (const Slider& slider : gSliders) {
        drawSlider(slider);
    }
    endOverlay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    updateProjection3D();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.15, gZoom, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    setLightColor();
    updateLightPosition();

    drawGround();
    drawLightMarker();

    if (gMode == 1) {
        drawModel(false);
    } else if (gMode == 2) {
        drawModel(true);
    } else {
        drawLitCube();
    }

    drawOverlay();
    glutSwapBuffers();
}

void reshape(int width, int height) {
    gWindowWidth = std::max(width, 1);
    gWindowHeight = std::max(height, 1);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    updateSliderLayout();
}

void idle() {
    const int tick = glutGet(GLUT_ELAPSED_TIME);
    const float dt = (gLastTickMs > 0) ? (tick - gLastTickMs) / 1000.0f : 0.0f;
    gLastTickMs = tick;

    if (gAutoRotate) {
        gLightAngle += dt * 1.1f;
        if (gLightAngle > kPi * 2.0f) {
            gLightAngle -= kPi * 2.0f;
        }
        gCubeAngle += dt * 34.0f;
        if (gCubeAngle > 360.0f) {
            gCubeAngle -= 360.0f;
        }
        if (gMode == 2) {
            gModelAutoYaw += dt * 8.0f;
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            glutLeaveMainLoop();
            break;
        case '1':
            gMode = 1;
            break;
        case '2':
            gMode = 2;
            break;
        case '3':
            gMode = 3;
            break;
        case ' ':
            gAutoRotate = !gAutoRotate;
            break;
        case '+':
        case '=':
            gZoom = std::max(1.4f, gZoom - 0.18f);
            break;
        case '-':
        case '_':
            gZoom = std::min(6.0f, gZoom + 0.18f);
            break;
        case 'b':
        case 'B':
            setMaterial(makeMaterial(0.33f, 0.22f, 0.03f, 0.78f, 0.57f, 0.11f, 0.99f, 0.91f, 0.81f, 27.8f), "Brass");
            break;
        case 'n':
        case 'N':
            setMaterial(makeMaterial(0.12f, 0.00f, 0.00f, 0.62f, 0.05f, 0.05f, 0.72f, 0.63f, 0.63f, 32.0f), "Red plastic");
            break;
        case 'm':
        case 'M':
            setMaterial(makeMaterial(0.50f, 0.50f, 0.50f, 0.92f, 0.92f, 0.88f, 1.0f, 1.0f, 1.0f, 96.0f), "Shiny white");
            break;
        case 'o':
        case 'O':
            gUseColoredLight = false;
            break;
        case 'p':
        case 'P':
            gUseColoredLight = true;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void special(int key, int, int) {
    if (key == GLUT_KEY_LEFT) {
        gModelYaw -= 5.0f;
    } else if (key == GLUT_KEY_RIGHT) {
        gModelYaw += 5.0f;
    } else if (key == GLUT_KEY_UP) {
        gModelPitch -= 4.0f;
    } else if (key == GLUT_KEY_DOWN) {
        gModelPitch += 4.0f;
    }
    gModelPitch = clampFloat(gModelPitch, -65.0f, 65.0f);
    glutPostRedisplay();
}

void updateSliderFromMouse(int index, int mouseX) {
    Slider& slider = gSliders[static_cast<std::size_t>(index)];
    const float t = clampFloat(static_cast<float>(mouseX - slider.x) / slider.width, 0.0f, 1.0f);
    slider.value = slider.minValue + t * (slider.maxValue - slider.minValue);
}

int hitSlider(int x, int y) {
    for (std::size_t i = 0; i < gSliders.size(); ++i) {
        const Slider& slider = gSliders[i];
        if (x >= slider.x && x <= slider.x + slider.width &&
            y >= slider.y - 6 && y <= slider.y + slider.height + 6) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        gActiveSlider = hitSlider(x, y);
        if (gActiveSlider >= 0) {
            updateSliderFromMouse(gActiveSlider, x);
        }
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        gActiveSlider = -1;
    } else if (button == 3 && state == GLUT_DOWN) {
        gZoom = std::max(1.4f, gZoom - 0.15f);
    } else if (button == 4 && state == GLUT_DOWN) {
        gZoom = std::min(6.0f, gZoom + 0.15f);
    }
    glutPostRedisplay();
}

void motion(int x, int) {
    if (gActiveSlider >= 0) {
        updateSliderFromMouse(gActiveSlider, x);
        glutPostRedisplay();
    }
}

void initScene() {
    glClearColor(0.96f, 0.96f, 0.94f, 1.0f);
    initLighting();
    initSliders();
    updateSliderLayout();
    setMaterial(makeMaterial(0.33f, 0.22f, 0.03f, 0.78f, 0.57f, 0.11f, 0.99f, 0.91f, 0.81f, 27.8f), "Brass");
}

}  // namespace

int main(int argc, char** argv) {
    gModelPath = findModelPath(argc, argv);
    gModelLoaded = loadPly(gModelPath);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Experiment 3 - 3D Model Display");

    initScene();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
