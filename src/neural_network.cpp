#include "neural_network.h"
#include <random>
#include <fstream>
#include <iostream>

NeuralNetwork::NeuralNetwork(const std::vector<int>& layerSizes)
    : layerSizes(layerSizes) {

    std::random_device rd;
    std::mt19937 gen(rd());

    // Initialize weights and biases for each layer
    for (size_t i = 0; i < layerSizes.size() - 1; i++) {
        int inputSize = layerSizes[i];
        int outputSize = layerSizes[i + 1];

        // Xavier/Glorot initialization for tanh activation
        // stddev = sqrt(2.0 / (inputSize + outputSize))
        float stddev = std::sqrt(2.0f / (inputSize + outputSize));
        std::normal_distribution<float> dist(0.0f, stddev);

        // Weight matrix: outputSize x inputSize
        Eigen::MatrixXf weight = Eigen::MatrixXf::Zero(outputSize, inputSize);
        for (int r = 0; r < outputSize; r++) {
            for (int c = 0; c < inputSize; c++) {
                weight(r, c) = dist(gen);
            }
        }
        weights.push_back(weight);

        // Bias vector: outputSize - initialize to small values
        Eigen::VectorXf bias = Eigen::VectorXf::Zero(outputSize);
        std::uniform_real_distribution<float> biasDist(-0.1f, 0.1f);
        for (int j = 0; j < outputSize; j++) {
            bias(j) = biasDist(gen);
        }
        biases.push_back(bias);
    }
}

std::vector<float> NeuralNetwork::forward(const std::vector<float>& input) {
    if (input.size() != layerSizes[0]) {
        std::cerr << "Ошибка: Несоответствие размера входа. Ожидается " << layerSizes[0]
                  << ", получено " << input.size() << std::endl;
        return std::vector<float>(layerSizes.back(), 0.0f);
    }

    // Convert input to Eigen vector
    Eigen::VectorXf activation(input.size());
    for (size_t i = 0; i < input.size(); i++) {
        activation(i) = input[i];
    }

    // Forward pass through each layer
    for (size_t i = 0; i < weights.size(); i++) {
        activation = weights[i] * activation + biases[i];
        activation = activate(activation);
    }

    // Convert output to std::vector
    std::vector<float> output(activation.size());
    for (int i = 0; i < activation.size(); i++) {
        output[i] = activation(i);
    }

    return output;
}

void NeuralNetwork::setWeights(const std::vector<Eigen::MatrixXf>& w,
                                const std::vector<Eigen::VectorXf>& b) {
    weights = w;
    biases = b;
}

void NeuralNetwork::mutate(float mutationRate, float mutationStrength) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::normal_distribution<float> mutateDist(0.0f, mutationStrength);

    // Mutate weights
    for (auto& weight : weights) {
        for (int r = 0; r < weight.rows(); r++) {
            for (int c = 0; c < weight.cols(); c++) {
                if (probDist(gen) < mutationRate) {
                    weight(r, c) += mutateDist(gen);
                }
            }
        }
    }

    // Mutate biases
    for (auto& bias : biases) {
        for (int i = 0; i < bias.size(); i++) {
            if (probDist(gen) < mutationRate) {
                bias(i) += mutateDist(gen);
            }
        }
    }
}

void NeuralNetwork::learnFromGradient(const std::vector<float>& lastInput,
                                        const std::vector<float>& desiredDirection,
                                        float learningRate) {
    if (lastInput.size() != layerSizes[0] || desiredDirection.size() != layerSizes.back()) {
        return; // Size mismatch
    }

    // Simple gradient-based learning: adjust output layer weights
    // This nudges the network towards producing outputs closer to desiredDirection

    // Convert to Eigen
    Eigen::VectorXf input(lastInput.size());
    for (size_t i = 0; i < lastInput.size(); i++) {
        input(i) = lastInput[i];
    }

    Eigen::VectorXf desired(desiredDirection.size());
    for (size_t i = 0; i < desiredDirection.size(); i++) {
        desired(i) = desiredDirection[i];
    }

    // Forward pass to get activations
    Eigen::VectorXf activation = input;
    std::vector<Eigen::VectorXf> activations;
    activations.push_back(activation);

    for (size_t i = 0; i < weights.size(); i++) {
        activation = weights[i] * activation + biases[i];
        activation = activate(activation);
        activations.push_back(activation);
    }

    // Backprop: adjust last layer to move towards desired output
    // Simple update: delta = learningRate * (desired - actual)
    Eigen::VectorXf outputError = desired - activations.back();

    // Update output layer weights and biases
    int lastLayer = weights.size() - 1;
    Eigen::VectorXf prevActivation = activations[lastLayer];

    // Weight update: w += learningRate * error * prevActivation^T
    weights[lastLayer] += learningRate * (outputError * prevActivation.transpose());

    // Bias update: b += learningRate * error
    biases[lastLayer] += learningRate * outputError;
}

NeuralNetwork NeuralNetwork::clone() const {
    NeuralNetwork copy(layerSizes);
    copy.setWeights(weights, biases);
    return copy;
}

void NeuralNetwork::save(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла для сохранения: " << filename << std::endl;
        return;
    }

    // Save layer sizes
    size_t numLayers = layerSizes.size();
    file.write(reinterpret_cast<const char*>(&numLayers), sizeof(numLayers));
    file.write(reinterpret_cast<const char*>(layerSizes.data()),
               numLayers * sizeof(int));

    // Save weights and biases
    for (size_t i = 0; i < weights.size(); i++) {
        int rows = weights[i].rows();
        int cols = weights[i].cols();
        file.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        file.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
        file.write(reinterpret_cast<const char*>(weights[i].data()),
                   rows * cols * sizeof(float));

        int biasSize = biases[i].size();
        file.write(reinterpret_cast<const char*>(&biasSize), sizeof(biasSize));
        file.write(reinterpret_cast<const char*>(biases[i].data()),
                   biasSize * sizeof(float));
    }

    file.close();
    std::cout << "Нейросеть сохранена в " << filename << std::endl;
}

void NeuralNetwork::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла для загрузки: " << filename << std::endl;
        return;
    }

    // Load layer sizes
    size_t numLayers;
    file.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));
    layerSizes.resize(numLayers);
    file.read(reinterpret_cast<char*>(layerSizes.data()),
              numLayers * sizeof(int));

    // Load weights and biases
    weights.clear();
    biases.clear();

    for (size_t i = 0; i < numLayers - 1; i++) {
        int rows, cols;
        file.read(reinterpret_cast<char*>(&rows), sizeof(rows));
        file.read(reinterpret_cast<char*>(&cols), sizeof(cols));

        Eigen::MatrixXf weight(rows, cols);
        file.read(reinterpret_cast<char*>(weight.data()),
                  rows * cols * sizeof(float));
        weights.push_back(weight);

        int biasSize;
        file.read(reinterpret_cast<char*>(&biasSize), sizeof(biasSize));
        Eigen::VectorXf bias(biasSize);
        file.read(reinterpret_cast<char*>(bias.data()),
                  biasSize * sizeof(float));
        biases.push_back(bias);
    }

    file.close();
    std::cout << "Нейросеть загружена из " << filename << std::endl;
}

int NeuralNetwork::getParameterCount() const {
    int count = 0;
    for (const auto& weight : weights) {
        count += weight.rows() * weight.cols();
    }
    for (const auto& bias : biases) {
        count += bias.size();
    }
    return count;
}

float NeuralNetwork::activate(float x) const {
    // tanh activation
    return std::tanh(x);
}

Eigen::VectorXf NeuralNetwork::activate(const Eigen::VectorXf& x) const {
    return x.array().tanh();
}
