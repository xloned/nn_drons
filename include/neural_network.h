#pragma once
#include <vector>
#include <string>
#include <Eigen/Dense>

// Simple feedforward neural network
class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<int>& layerSizes);

    // Forward pass: input -> output
    std::vector<float> forward(const std::vector<float>& input);

    // Get/Set weights (for RL training)
    std::vector<Eigen::MatrixXf> getWeights() const { return weights; }
    std::vector<Eigen::VectorXf> getBiases() const { return biases; }
    void setWeights(const std::vector<Eigen::MatrixXf>& w, const std::vector<Eigen::VectorXf>& b);

    // Mutate weights slightly (for evolutionary approach)
    void mutate(float mutationRate, float mutationStrength);

    // Learn from gradient: nudge weights towards better behavior
    // direction: desired direction vector for output adjustment
    void learnFromGradient(const std::vector<float>& lastInput,
                           const std::vector<float>& desiredDirection,
                           float learningRate);

    // Clone network
    NeuralNetwork clone() const;

    // Save/Load weights
    void save(const std::string& filename) const;
    void load(const std::string& filename);

    // Get total number of parameters
    int getParameterCount() const;

private:
    std::vector<int> layerSizes;
    std::vector<Eigen::MatrixXf> weights;  // Weight matrices for each layer
    std::vector<Eigen::VectorXf> biases;   // Bias vectors for each layer

    // Activation function (tanh)
    float activate(float x) const;
    Eigen::VectorXf activate(const Eigen::VectorXf& x) const;
};
