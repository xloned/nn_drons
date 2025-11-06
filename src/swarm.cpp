#include "swarm.h"
#include <random>
#include <iostream>
#include <iomanip>

Swarm::Swarm(int numDrones)
    : numDrones(numDrones), generation(0), bestFitness(0.0f),
      episodeTime(0.0f), maxEpisodeTime(40.0f) {  // INCREASED to 40s - ОЧЕНЬ СЛОЖНАЯ задача!

    // Fixed starting position for all drones (they all start from the same point)
    // МАКСИМАЛЬНО ДАЛЕКО - старт очень далеко от стены!
    Vec3 fixedStartPos(0.0f, 0.0f, -35.0f); // Center, 35 units behind the wall (was -15, now MORE THAN 2X!)

    for (int i = 0; i < numDrones; i++) {
        drones.push_back(std::make_shared<Drone>(fixedStartPos));

        // Create neural network for each drone
        // Input: 22 sensors (was 18), Hidden: 24, 16, Output: 4 (control signals)
        // Increased hidden layer size for more learning capacity
        std::vector<int> layerSizes = {22, 24, 16, 4};
        auto network = std::make_shared<NeuralNetwork>(layerSizes);

        // ВАЖНО: Добавляем небольшую случайную мутацию для РАЗНООБРАЗИЯ
        // Иначе все дроны летят одинаково!
        if (i > 0) {  // Первый дрон без мутации
            network->mutate(0.3f, 0.5f);  // Сильная начальная мутация для разнообразия
        }

        networks.push_back(network);
        fitnessScores.push_back(0.0f);
    }

    environment.reset();
}

void Swarm::reset() {
    // DON'T reset environment - keep the same hole position!
    // environment.reset();  // Commented out - hole stays in same place

    // Fixed starting position for all drones (same point every time)
    Vec3 fixedStartPos(0.0f, 0.0f, -35.0f);  // ИСПРАВЛЕНО: было -15, теперь -35 как в конструкторе!

    // Reset all drones to the same starting position
    for (auto& drone : drones) {
        drone->reset(fixedStartPos);
        drone->clearTrajectory(); // Clear trajectory history
    }

    // Reset fitness scores
    for (auto& score : fitnessScores) {
        score = 0.0f;
    }

    episodeTime = 0.0f;

    std::cout << "Reset complete - " << drones.size() << " drones ready at origin" << std::endl;
}

void Swarm::update(float dt) {
    episodeTime += dt;

    // FIRST: Check if any drone already succeeded - if so, STOP immediately!
    if (hasAnyDroneSucceeded()) {
        return; // Don't update anything - success achieved!
    }

    // Update each drone
    for (size_t i = 0; i < drones.size(); i++) {
        auto& drone = drones[i];

        if (!drone->isActive()) {
            continue;
        }

        // Get sensor readings
        std::vector<float> sensors = drone->getSensorReadings(environment);

        // Record trajectory for learning
        drone->recordStep(sensors);

        // Get control from neural network
        std::vector<float> control = networks[i]->forward(sensors);

        // Apply control
        drone->applyControl(control);

        // Update physics
        drone->update(dt);

        // Check if passed through hole
        // New logic: check if drone is near wall AND in hole area
        Vec3 pos = drone->getPosition();
        float wallZ = environment.getWallZ();

        // Check if drone is crossing the wall (from -0.5 to +1.0 units)
        if (pos.z > wallZ - 0.5f && pos.z < wallZ + 1.0f && !drone->isSuccessful()) {
            if (environment.isInHole(pos)) {
                drone->setSuccessful(true);
                drone->setActive(false);
                fitnessScores[i] += trainer.calculateReward(*drone, environment, true, false);
                std::cout << "\n🎉 🎉 🎉 УСПЕХ! Дрон " << i << " нашёл дыру! 🎉 🎉 🎉" << std::endl;
                std::cout << "Позиция: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
                std::cout << "Центр дыры: (" << environment.getHoleCenter().x << ", "
                          << environment.getHoleCenter().y << ", " << environment.getHoleCenter().z << ")" << std::endl;
                std::cout << "Поколение: " << generation << std::endl;
                std::cout << "Время: " << episodeTime << "с" << std::endl;

                // LEARN FROM SUCCESS - apply gradient-based learning!
                learnFromSuccessfulTrajectory(i);

                // EXIT IMMEDIATELY - don't process other drones!
                return;
            }
        }

        // Check for collision with wall
        if (drone->hasCollided(environment)) {
            drone->setActive(false);
            fitnessScores[i] += trainer.calculateReward(*drone, environment, false, true);
        }

        // Check if drone went out of bounds (flew away)
        if (environment.isOutOfBounds(drone->getPosition())) {
            drone->setActive(false);
            fitnessScores[i] += trainer.calculateReward(*drone, environment, false, true);
            // Note: treating out of bounds same as collision
        }

        // Update fitness continuously
        if (drone->isActive()) {
            fitnessScores[i] += trainer.calculateReward(*drone, environment, false, false) * dt;
        }
    }

    // Check if episode is over (time limit or all drones inactive)
    bool allInactive = true;
    for (const auto& drone : drones) {
        if (drone->isActive()) {
            allInactive = false;
            break;
        }
    }

    // Check if any drone succeeded (VICTORY CONDITION!)
    bool anySuccess = false;
    for (const auto& drone : drones) {
        if (drone->isSuccessful()) {
            anySuccess = true;
            break;
        }
    }

    if (anySuccess) {
        // SUCCESS! At least one drone found the hole - STOP SIMULATION!
        std::cout << "\n🎉 🎉 🎉 УСПЕХ! Дрон нашёл дыру! 🎉 🎉 🎉" << std::endl;
        std::cout << "Поколение: " << generation << std::endl;
        std::cout << "Время: " << episodeTime << "с" << std::endl;
        std::cout << "Обучение завершено!" << std::endl;
        // Don't reset - let the main loop handle exit
        return;
    }

    if (allInactive || episodeTime >= maxEpisodeTime) {
        // Episode failed - no drone found the hole, try again
        std::string reason = (episodeTime >= maxEpisodeTime) ? "ВРЕМЯ ВЫШЛО" : "ВСЕ СТОЛКНУЛИСЬ";

        std::cout << "\n=== Поколение " << generation << " - " << reason << " ===" << std::endl;
        std::cout << "Длительность: " << std::fixed << std::setprecision(1) << episodeTime
                  << "с / " << maxEpisodeTime << "с" << std::endl;

        int collisionCount = 0;
        for (const auto& drone : drones) {
            if (!drone->isActive()) collisionCount++;
        }
        std::cout << "Неактивных дронов: " << collisionCount << "/" << drones.size() << std::endl;

        // Find and show best drone
        int bestIdx = trainer.getBestNetworkIndex(fitnessScores);

        // Calculate average fitness
        float avgFitness = 0.0f;
        for (float fitness : fitnessScores) {
            avgFitness += fitness;
        }
        avgFitness /= fitnessScores.size();

        std::cout << "Лучший дрон: D" << bestIdx << " - Результат: " << std::fixed
                  << std::setprecision(1) << fitnessScores[bestIdx] << std::endl;
        std::cout << "Средний результат: " << std::fixed << std::setprecision(1)
                  << avgFitness << std::endl;

        // Show fitness scores for debugging (only if few drones)
        if (drones.size() <= 10) {
            std::cout << "Все результаты: ";
            for (size_t i = 0; i < fitnessScores.size(); i++) {
                std::cout << "D" << i << "=" << std::fixed << std::setprecision(0)
                          << fitnessScores[i] << " ";
            }
            std::cout << std::endl;
        }

        // Train and reset to try again
        trainNetworks();
        reset();
        generation++;

        std::cout << "\n>>> Запуск поколения " << generation << "...\n" << std::endl;
    }
}

int Swarm::getSuccessfulDroneIndex() const {
    for (size_t i = 0; i < drones.size(); i++) {
        if (drones[i]->isSuccessful()) {
            return i;
        }
    }
    return -1;
}

void Swarm::coordinateTowardsSuccess(int successfulDroneIdx) {
    // When one drone finds the hole, others move towards it
    Vec3 targetPos = drones[successfulDroneIdx]->getPosition();

    for (size_t i = 0; i < drones.size(); i++) {
        if (i == successfulDroneIdx || !drones[i]->isActive()) {
            continue;
        }

        // Add attractive force towards successful drone
        Vec3 toTarget = targetPos - drones[i]->getPosition();
        Vec3 direction = toTarget.normalized();

        // Simple coordination: apply control towards successful drone
        std::vector<float> coordControl = {direction.x, direction.y, direction.z, 1.0f};
        drones[i]->applyControl(coordControl);
    }
}

void Swarm::trainNetworks() {
    trainer.trainStep(networks, fitnessScores);

    // Update best fitness
    int bestIdx = trainer.getBestNetworkIndex(fitnessScores);
    if (fitnessScores[bestIdx] > bestFitness) {
        bestFitness = fitnessScores[bestIdx];
        std::cout << "New best fitness: " << bestFitness << " at generation " << generation << std::endl;
    }
}

float Swarm::calculateFitness(int droneIdx) {
    return fitnessScores[droneIdx];
}

void Swarm::saveBestNetwork(const std::string& filename) {
    int bestIdx = trainer.getBestNetworkIndex(fitnessScores);
    networks[bestIdx]->save(filename);
}

void Swarm::loadNetwork(const std::string& filename) {
    // Load network into all drones
    networks[0]->load(filename);
    for (size_t i = 1; i < networks.size(); i++) {
        *networks[i] = networks[0]->clone();
    }
}

bool Swarm::hasAnyDroneSucceeded() const {
    for (const auto& drone : drones) {
        if (drone->isSuccessful()) {
            return true;
        }
    }
    return false;
}

void Swarm::learnFromSuccessfulTrajectory(int successfulDroneIdx) {
    // Get successful drone's trajectory
    const auto& trajectory = drones[successfulDroneIdx]->getTrajectory();

    if (trajectory.empty()) {
        return;
    }

    std::cout << "Обучение на успешной траектории (" << trajectory.size() << " шагов)..." << std::endl;

    // Learning rate - small adjustments
    float learningRate = 0.01f;

    // Go through trajectory and learn: at each step, teach network to move towards hole
    for (size_t step = 0; step < trajectory.size(); step++) {
        const auto& sensors = trajectory[step];

        // Calculate desired direction (towards hole)
        // Sensors 6-8 contain direction to hole (already normalized)
        if (sensors.size() >= 11) {
            std::vector<float> desiredControl = {
                sensors[6],  // dirToHole.x
                sensors[7],  // dirToHole.y
                sensors[8],  // dirToHole.z
                1.0f         // forward thrust
            };

            // Apply learning to successful drone's network
            networks[successfulDroneIdx]->learnFromGradient(sensors, desiredControl, learningRate);
        }
    }

    std::cout << "Обучение завершено! Нейросеть скорректирована на основе успешного пути." << std::endl;

    // Now copy this improved network to ALL other drones
    for (size_t i = 0; i < networks.size(); i++) {
        if (i != successfulDroneIdx) {
            *networks[i] = networks[successfulDroneIdx]->clone();
        }
    }

    std::cout << "Знания переданы всем " << networks.size() << " дронам!" << std::endl;
}
