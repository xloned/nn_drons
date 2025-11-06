#pragma once
#include "drone.h"
#include "neural_network.h"
#include "environment.h"
#include "rl_trainer.h"
#include <vector>
#include <memory>

// Manages the swarm of drones
class Swarm {
public:
    Swarm(int numDrones);

    // Reset all drones and environment
    void reset();

    // Update all drones
    void update(float dt);

    // Check if any drone found the hole (for swarm coordination)
    int getSuccessfulDroneIndex() const;

    // Make other drones fly towards successful drone
    void coordinateTowardsSuccess(int successfulDroneIdx);

    // Train the neural networks
    void trainNetworks();

    // Learn from successful drone's trajectory
    void learnFromSuccessfulTrajectory(int successfulDroneIdx);

    // Getters
    const std::vector<std::shared_ptr<Drone>>& getDrones() const { return drones; }
    const Environment& getEnvironment() const { return environment; }
    int getGeneration() const { return generation; }
    float getBestFitness() const { return bestFitness; }
    float getEpisodeTime() const { return episodeTime; }
    float getMaxEpisodeTime() const { return maxEpisodeTime; }
    bool hasAnyDroneSucceeded() const; // Check if any drone found the hole

    // Save/load best network
    void saveBestNetwork(const std::string& filename);
    void loadNetwork(const std::string& filename);

private:
    std::vector<std::shared_ptr<Drone>> drones;
    std::vector<std::shared_ptr<NeuralNetwork>> networks;
    std::vector<float> fitnessScores;

    Environment environment;
    RLTrainer trainer;

    int numDrones;
    int generation;
    float bestFitness;
    float episodeTime;
    float maxEpisodeTime;

    // Calculate fitness for a drone
    float calculateFitness(int droneIdx);
};
