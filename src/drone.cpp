#include "drone.h"
#include "neural_network.h"
#include <cmath>

Drone::Drone(const Vec3& startPos)
    : position(startPos), velocity(0, 0, 0), radius(0.5f),
      active(true), successful(false) {
}

void Drone::reset(const Vec3& startPos) {
    position = startPos;
    velocity = Vec3(0, 0, 0);
    active = true;
    successful = false;
}

void Drone::update(float dt) {
    if (!active) return;

    // Simple physics: update position based on velocity
    position += velocity * dt;

    // Apply LESS damping - let drones maintain momentum better (prevents early stopping)
    velocity = velocity * 0.995f;  // Changed from 0.98 to 0.995 - less friction
}

void Drone::applyControl(const std::vector<float>& control) {
    if (!active || control.size() < 4) return;

    // Control is 4 values: force in X, Y, Z directions, and forward thrust
    // Simple model: directly adjust velocity (no mass/acceleration for simplicity)
    float maxSpeed = 10.0f;       // Max speed
    float controlStrength = 1.5f; // INCREASED from 1.0 to 1.5 - more responsive control!

    Vec3 controlVec(control[0], control[1], control[2]);

    // Add control to velocity with increased strength
    velocity += controlVec * controlStrength;

    // Forward bias - помощь дронам двигаться к стене (положительное направление Z)
    velocity.z += 0.06f;  // Оптимальный баланс - не слишком сильно, но помогает

    // Clamp velocity
    if (velocity.lengthSquared() > maxSpeed * maxSpeed) {
        velocity = velocity.normalized() * maxSpeed;
    }
}

std::vector<float> Drone::getSensorReadings(const Environment& env) const {
    std::vector<float> sensors;

    // 1. Drone's own position (3 values)
    sensors.push_back(position.x / 10.0f);  // Normalize to roughly [-1, 1]
    sensors.push_back(position.y / 10.0f);
    sensors.push_back(position.z / 10.0f);

    // 2. Drone's velocity (3 values)
    sensors.push_back(velocity.x / 5.0f);
    sensors.push_back(velocity.y / 5.0f);
    sensors.push_back(velocity.z / 5.0f);

    // 3. Direction to hole center (3 values)
    Vec3 toHole = env.getHoleCenter() - position;
    float distToHole = toHole.length();
    Vec3 dirToHole(0, 0, 0);
    if (distToHole > 0.001f) {
        dirToHole = toHole.normalized();
        sensors.push_back(dirToHole.x);
        sensors.push_back(dirToHole.y);
        sensors.push_back(dirToHole.z);
    } else {
        sensors.push_back(0);
        sensors.push_back(0);
        sensors.push_back(0);
    }

    // 4. Distance to hole (1 value)
    sensors.push_back(distToHole / 20.0f);

    // 5. Distance to wall (1 value) - НОВОЕ! Важно для избежания столкновений
    float distToWall = std::abs(position.z - env.getWallZ());
    sensors.push_back(distToWall / 15.0f); // Normalize

    // 6. Alignment with hole (1 value) - как хорошо мы нацелены на дыру
    // Dot product между направлением движения и направлением к дыре
    float alignment = 0.0f;
    if (velocity.lengthSquared() > 0.001f && distToHole > 0.001f) {
        Vec3 velDir = velocity.normalized();
        alignment = velDir.dot(dirToHole);
    }
    sensors.push_back(alignment);

    // 7. Offset from hole in XY plane (2 values) - насколько мы смещены от дыры
    Vec3 holePos = env.getHoleCenter();
    sensors.push_back((position.x - holePos.x) / 10.0f);
    sensors.push_back((position.y - holePos.y) / 10.0f);

    // 8. Ray cast sensors in 8 directions (8 values)
    // Front, back, left, right, up, down, and 2 diagonals
    std::vector<Vec3> rayDirections = {
        Vec3(1, 0, 0),   // Right
        Vec3(-1, 0, 0),  // Left
        Vec3(0, 1, 0),   // Up
        Vec3(0, -1, 0),  // Down
        Vec3(0, 0, 1),   // Forward
        Vec3(0, 0, -1),  // Back
        Vec3(1, 1, 0).normalized(),   // Diagonal
        Vec3(-1, -1, 0).normalized()  // Diagonal
    };

    for (const auto& dir : rayDirections) {
        sensors.push_back(castRay(dir, env));
    }

    // Total: 3 + 3 + 3 + 1 + 1 + 1 + 2 + 8 = 22 input values
    return sensors;
}

float Drone::castRay(const Vec3& direction, const Environment& env) const {
    // Simple ray casting: distance to wall in given direction
    // Returns normalized distance [0, 1]

    float maxDist = 20.0f;
    Vec3 wallNormal(0, 0, 1);
    float wallZ = env.getWallZ();

    // If ray is parallel to wall, return max distance
    float denom = direction.dot(wallNormal);
    if (std::abs(denom) < 0.001f) {
        return 1.0f;
    }

    // Calculate intersection with wall plane
    float t = (wallZ - position.z) / direction.z;

    if (t < 0) {
        return 1.0f; // Behind us
    }

    // Distance is normalized
    return std::min(t / maxDist, 1.0f);
}

bool Drone::hasPassedThroughHole(const Environment& env) const {
    // Check if drone is crossing or just past the wall (not too far!)
    float wallZ = env.getWallZ();
    // Check if we're in the zone where we should check for hole passage
    // From 0.3 units before wall to 0.5 units after
    return position.z > wallZ - 0.3f && position.z < wallZ + 0.5f && !successful;
}

bool Drone::hasCollided(const Environment& env) const {
    return env.collidesWithWall(position, radius);
}
