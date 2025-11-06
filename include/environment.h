#pragma once
#include "vec3.h"
#include <random>

// Represents the wall with a hole
class Environment {
public:
    Environment();

    // Reset environment with random hole position
    void reset();

    // Check if point is inside the hole
    bool isInHole(const Vec3& position) const;

    // Check if point collides with wall (but not in hole)
    bool collidesWithWall(const Vec3& position, float droneRadius) const;

    // Check if drone is out of bounds
    bool isOutOfBounds(const Vec3& position) const;

    // Getters
    Vec3 getHoleCenter() const { return holeCenter; }
    float getHoleRadius() const { return holeRadius; }
    float getWallZ() const { return wallZ; }
    Vec3 getBoundsMin() const { return boundsMin; }
    Vec3 getBoundsMax() const { return boundsMax; }

private:
    Vec3 holeCenter;      // Center of the hole in 3D space
    float holeRadius;     // Radius of the hole
    float wallZ;          // Z position of the wall
    Vec3 boundsMin;       // Minimum bounds of the environment
    Vec3 boundsMax;       // Maximum bounds of the environment

    std::mt19937 rng;
};
