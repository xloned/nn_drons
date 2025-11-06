#pragma once
#include "neural_network.h"
#include "drone.h"
#include "environment.h"
#include <vector>
#include <memory>

// Handles reinforcement learning training
class RLTrainer {
public:
    RLTrainer();

    // Calculate reward for a drone's current state
    float calculateReward(const Drone& drone, const Environment& env, bool reachedGoal, bool collided) const;

    // Train using simple evolutionary strategy
    void trainStep(std::vector<std::shared_ptr<NeuralNetwork>>& networks,
                   const std::vector<float>& fitnessScores);

    // Get best network index
    int getBestNetworkIndex(const std::vector<float>& fitnessScores) const;

private:
    float mutationRate;
    float mutationStrength;
};
