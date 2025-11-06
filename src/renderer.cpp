#include "renderer.h"
#include <iostream>
#include <cmath>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

Renderer::Renderer(int width, int height)
    : window(nullptr), width(width), height(height),
      cameraDistance(50.0f), cameraAngleX(10.0f), cameraAngleY(0.0f) {  // INCREASED from 25 to 50 - дроны стартуют на -35!
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool Renderer::init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW" << std::endl;
        return false;
    }

    // Create window
    window = glfwCreateWindow(width, height, "Дроны с Нейросетью - Поиск Дыры", nullptr, nullptr);
    if (!window) {
        std::cerr << "Ошибка создания окна GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Light properties
    GLfloat lightPos[] = {10.0f, 10.0f, 10.0f, 1.0f};
    GLfloat lightAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    return true;
}

void Renderer::render(const Swarm& swarm) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup camera
    setupCamera();

    // Draw environment
    drawWall(swarm.getEnvironment());
    drawHole(swarm.getEnvironment());

    // Draw drones
    for (const auto& drone : swarm.getDrones()) {
        drawDrone(*drone);
    }

    // Draw info text (generation, best fitness)
    // Note: Text rendering in OpenGL is complex, so we'll skip for now
    // You can add it later using a library like FreeType

    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Renderer::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Camera controls
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraAngleY -= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraAngleY += 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        cameraAngleX -= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraAngleX += 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraDistance -= 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraDistance += 0.5f;
    }
}

void Renderer::setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)width / (double)height, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera is behind the drones, looking at the center between drones and wall
    // Wall is at z=0, drones spawn at z=-35 area
    // Camera position based on angles (for user control)
    float camX = cameraAngleY * 0.1f; // Side movement
    float camY = 5.0f + cameraAngleX * 0.1f; // Height with adjustment
    float camZ = -cameraDistance; // Behind the drones

    // Look at center between drones (-35) and wall (0) = -17.5
    gluLookAt(camX, camY, camZ,     // Camera position (behind drones)
              0.0, 0.0, -17.5,       // Look at center of action (was 0,0,0)
              0.0, 1.0, 0.0);        // Up vector
}

void Renderer::drawSphere(const Vec3& position, float radius, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glColor3f(r, g, b);

    // Draw sphere using quad strips (simple method)
    const int slices = 16;
    const int stacks = 16;

    GLUquadric* quad = gluNewQuadric();
    gluSphere(quad, radius, slices, stacks);
    gluDeleteQuadric(quad);

    glPopMatrix();
}

void Renderer::drawWall(const Environment& env) {
    glPushMatrix();

    float wallZ = env.getWallZ();
    Vec3 min = env.getBoundsMin();
    Vec3 max = env.getBoundsMax();

    // Draw semi-transparent wall
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.6f, 0.6f, 0.7f, 0.5f);

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(min.x, min.y, wallZ);
    glVertex3f(max.x, min.y, wallZ);
    glVertex3f(max.x, max.y, wallZ);
    glVertex3f(min.x, max.y, wallZ);
    glEnd();

    glDisable(GL_BLEND);

    glPopMatrix();
}

void Renderer::drawHole(const Environment& env) {
    Vec3 center = env.getHoleCenter();
    float radius = env.getHoleRadius();

    // Draw hole as a bright circle
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);

    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 1.0f, 0.2f);

    // Draw circle
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) {
        float angle = i * 2.0f * M_PI / 32.0f;
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        glVertex3f(x, y, 0);
    }
    glEnd();

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void Renderer::drawDrone(const Drone& drone) {
    Vec3 pos = drone.getPosition();
    float radius = drone.getRadius();

    // Color based on status
    float r, g, b;
    if (drone.isSuccessful()) {
        r = 0.2f; g = 1.0f; b = 0.2f; // Green for successful
    } else if (!drone.isActive()) {
        r = 1.0f; g = 0.2f; b = 0.2f; // Red for failed
    } else {
        r = 0.3f; g = 0.5f; b = 1.0f; // Blue for active
    }

    drawSphere(pos, radius, r, g, b);
}

void Renderer::setWindowTitle(const std::string& title) {
    if (window) {
        glfwSetWindowTitle(window, title.c_str());
    }
}
