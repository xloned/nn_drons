#pragma once
#include "swarm.h"
#include <GLFW/glfw3.h>
#include <string>

// Handles OpenGL rendering
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    // Initialize OpenGL context
    bool init();

    // Render the scene
    void render(const Swarm& swarm);

    // Check if window should close
    bool shouldClose() const;

    // Process input
    void processInput();

    // Get window
    GLFWwindow* getWindow() const { return window; }

    // Set window title (for success message)
    void setWindowTitle(const std::string& title);

private:
    GLFWwindow* window;
    int width, height;

    // Camera parameters
    float cameraDistance;
    float cameraAngleX;
    float cameraAngleY;

    // Draw primitives
    void drawSphere(const Vec3& position, float radius, float r, float g, float b);
    void drawWall(const Environment& env);
    void drawHole(const Environment& env);
    void drawDrone(const Drone& drone);

    // Setup camera
    void setupCamera();
};
