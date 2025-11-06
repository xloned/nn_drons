#pragma once
#include "vec3.h"
#include "environment.h"
#include <vector>
#include <memory>

class NeuralNetwork;

// Represents a single drone
class Drone {
public:
    Drone(const Vec3& startPos);

    // Reset drone to starting position
    void reset(const Vec3& startPos);

    // Update physics (simple: velocity-based movement)
    void update(float dt);

    // Apply control from neural network output
    void applyControl(const std::vector<float>& control);

    // Get sensor readings for neural network input
    std::vector<float> getSensorReadings(const Environment& env) const;

    // Check if drone passed through hole
    bool hasPassedThroughHole(const Environment& env) const;

    // Check if drone collided with wall
    bool hasCollided(const Environment& env) const;

    // Getters
    Vec3 getPosition() const { return position; }
    Vec3 getVelocity() const { return velocity; }
    float getRadius() const { return radius; }
    bool isActive() const { return active; }
    bool isSuccessful() const { return successful; }

    // Get trajectory history for learning
    const std::vector<std::vector<float>>& getTrajectory() const { return trajectory; }
    void recordStep(const std::vector<float>& sensors) {
        trajectory.push_back(sensors);
    }
    void clearTrajectory() { trajectory.clear(); }

    // Setters
    void setActive(bool val) { active = val; }
    void setSuccessful(bool val) { successful = val; }

private:
    Vec3 position;
    Vec3 velocity;
    float radius;
    bool active;      // Still trying to find hole
    bool successful;  // Found the hole

    // Store trajectory for learning from successful runs
    std::vector<std::vector<float>> trajectory;

    // Cast a ray and return distance to wall
    float castRay(const Vec3& direction, const Environment& env) const;
};
