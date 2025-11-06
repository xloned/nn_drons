#include "environment.h"
#include <random>

Environment::Environment()
    : wallZ(0.0f), holeRadius(1.0f),
      boundsMin(-12, -12, -40), boundsMax(12, 12, 10),  // ИСПРАВЛЕНО: было -20, теперь -40 (дроны стартуют на -35!)
      rng(std::random_device{}()) {
    reset();
}

void Environment::reset() {
    // Wall is at z = 0
    wallZ = 0.0f;

    // Randomize hole position on the wall (x, y plane)
    // МАКСИМАЛЬНО УВЕЛИЧЕННЫЙ РАЗБРОС - дыра может быть где угодно на стене!
    std::uniform_real_distribution<float> distX(-10.0f, 10.0f);  // Was -6 to 6, now HUGE range!
    std::uniform_real_distribution<float> distY(-10.0f, 10.0f);  // Was -6 to 6, now HUGE range!

    holeCenter = Vec3(distX(rng), distY(rng), wallZ);

    // ОЧЕНЬ МАЛЕНЬКАЯ ДЫРА - всего 1.2x размера дрона!
    // Дроны имеют радиус 0.5, дыра теперь только 1.2x = очень точное прохождение!
    holeRadius = 0.6f;  // Was 1.0f (2x), now 0.6f (1.2x) - ОЧЕНЬ СЛОЖНО!
}

bool Environment::isInHole(const Vec3& position) const {
    // Check if position is near the wall plane
    if (std::abs(position.z - wallZ) > 0.5f) {
        return false;
    }

    // Check if position is within hole radius
    float dx = position.x - holeCenter.x;
    float dy = position.y - holeCenter.y;
    float distSq = dx * dx + dy * dy;

    return distSq <= (holeRadius * holeRadius);
}

bool Environment::collidesWithWall(const Vec3& position, float droneRadius) const {
    // If we're on the other side of the wall (passed through), no collision
    if (position.z > wallZ + 0.5f) {
        return false;
    }

    // If we're far from the wall, no collision
    if (position.z < wallZ - 1.0f) {
        return false;
    }

    // We're near the wall, check if we're in the hole
    if (isInHole(position)) {
        return false;
    }

    // We're near the wall but not in hole = collision
    return std::abs(position.z - wallZ) < droneRadius;
}

bool Environment::isOutOfBounds(const Vec3& position) const {
    // Check if drone is outside the boundaries
    return position.x < boundsMin.x || position.x > boundsMax.x ||
           position.y < boundsMin.y || position.y > boundsMax.y ||
           position.z < boundsMin.z || position.z > boundsMax.z;
}
