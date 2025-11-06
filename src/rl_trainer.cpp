#include "rl_trainer.h"
#include <algorithm>
#include <cmath>

RLTrainer::RLTrainer()
    : mutationRate(0.05f), mutationStrength(0.1f) {
    // Balanced mutation for exploration and exploitation
}

float RLTrainer::calculateReward(const Drone& drone, const Environment& env,
                                  bool reachedGoal, bool collided) const {
    float reward = 0.0f;

    if (reachedGoal) {
        // HUGE positive reward for reaching goal
        reward += 2000.0f;
    } else if (collided) {
        // Small negative reward for collision
        reward -= 10.0f;

        // But still reward based on how close we got
        Vec3 toHole = env.getHoleCenter() - drone.getPosition();
        float distance = toHole.length();
        reward += std::max(0.0f, (25.0f - distance) * 3.0f);
    } else {
        // Reward based on proximity to hole
        Vec3 toHole = env.getHoleCenter() - drone.getPosition();
        float distance = toHole.length();

        // Distance-based reward (quadratic for stronger gradient near hole)
        float proximityReward = std::max(0.0f, (25.0f - distance));
        reward += proximityReward * proximityReward * 0.15f; // Quadratic reward

        // Bonus for XY alignment - если мы точно над/под дырой в XY плоскости
        Vec3 holePos = env.getHoleCenter();
        float xyDistance = std::sqrt((drone.getPosition().x - holePos.x) * (drone.getPosition().x - holePos.x) +
                                     (drone.getPosition().y - holePos.y) * (drone.getPosition().y - holePos.y));
        float holeRadius = env.getHoleRadius();

        if (xyDistance < holeRadius * 2.0f) {
            // Very close to hole in XY plane - big bonus!
            reward += (1.0f - xyDistance / (holeRadius * 2.0f)) * 15.0f;
        }

        // IMPROVED: Bonus for moving towards hole (INCREASED reward and added penalty for wrong direction)
        Vec3 velocity = drone.getVelocity();
        if (velocity.lengthSquared() > 0.01f && distance > 0.01f) {
            Vec3 velDir = velocity.normalized();
            Vec3 toHoleDir = toHole.normalized();
            float alignment = velDir.dot(toHoleDir);
            if (alignment > 0) {
                // Much stronger reward for moving in right direction!
                reward += alignment * 8.0f;  // Increased from 2.0 to 8.0
            } else {
                // PENALTY for moving AWAY from hole (prevents loops)
                reward += alignment * 4.0f;  // negative alignment = negative reward
            }
        }

        // ANTI-LOOP PENALTY: Penalize high speed when far from hole (likely looping)
        float speed = std::sqrt(velocity.lengthSquared());
        if (speed > 3.0f && distance > 5.0f) {
            // Moving fast but still far from hole = probably looping
            float loopPenalty = speed * 0.5f;
            reward -= loopPenalty;
        }

        // Time penalty to encourage speed (increased from 0.005)
        reward -= 0.02f;
    }

    return reward;
}

void RLTrainer::trainStep(std::vector<std::shared_ptr<NeuralNetwork>>& networks,
                          const std::vector<float>& fitnessScores) {
    if (networks.empty() || fitnessScores.size() != networks.size()) {
        return;
    }

    // Find best network
    int bestIdx = getBestNetworkIndex(fitnessScores);
    auto bestNetwork = networks[bestIdx];

    // IMPROVED STRATEGY: Elite selection + multi-modal diversity
    // Keep best unchanged, create variations with different mutation strategies

    for (size_t i = 0; i < networks.size(); i++) {
        if (i == bestIdx) {
            continue; // Keep best network unchanged (elitism)
        }

        // Clone best network
        *networks[i] = bestNetwork->clone();

        // Different mutation strategies for different drones:
        // This creates a diverse population that can both exploit and explore
        if (i == 0) {
            // Very small mutations - fine-tune the best solution
            networks[i]->mutate(mutationRate * 0.3f, mutationStrength * 0.3f);
        } else if (i == 1) {
            // Small-medium mutations - local exploitation
            networks[i]->mutate(mutationRate * 0.6f, mutationStrength * 0.6f);
        } else if (i == networks.size() - 1) {
            // Very large mutation - aggressive exploration
            networks[i]->mutate(mutationRate * 3.0f, mutationStrength * 3.0f);
        } else if (i == networks.size() - 2) {
            // Large mutation - exploration
            networks[i]->mutate(mutationRate * 1.8f, mutationStrength * 1.8f);
        } else {
            // Normal mutation - balanced
            networks[i]->mutate(mutationRate, mutationStrength);
        }
    }
}

int RLTrainer::getBestNetworkIndex(const std::vector<float>& fitnessScores) const {
    if (fitnessScores.empty()) {
        return 0;
    }

    return std::max_element(fitnessScores.begin(), fitnessScores.end())
           - fitnessScores.begin();
}
