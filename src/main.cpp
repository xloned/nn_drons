#include "renderer.h"
#include "swarm.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    std::cout << "=== Дроны с Нейросетями - Симуляция Поиска Дыры ===" << std::endl;
    std::cout << "Управление:" << std::endl;
    std::cout << "  Стрелки: Вращение камеры" << std::endl;
    std::cout << "  W/S: Приближение/отдаление" << std::endl;
    std::cout << "  ESC: Выход" << std::endl;
    std::cout << "========================================================" << std::endl;

    // Create swarm with 100 drones for better learning
    int numDrones = 100;
    Swarm swarm(numDrones);

    // Check if we should load a saved network
    bool loadNetwork = false;
    std::string networkFile = "best_network.bin";

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--load" || arg == "-l") {
            loadNetwork = true;
            std::cout << "Загрузка сохранённой нейросети из " << networkFile << std::endl;
            swarm.loadNetwork(networkFile);
        }
    }

    // Create renderer
    Renderer renderer(800, 600);
    if (!renderer.init()) {
        std::cerr << "Ошибка инициализации рендерера" << std::endl;
        return -1;
    }

    std::cout << "Запуск симуляции..." << std::endl;

    // Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float fpsTimer = 0.0f;

    // Fixed timestep for stable simulation
    const float targetDt = 1.0f / 60.0f; // 60 FPS target

    while (!renderer.shouldClose() && !swarm.hasAnyDroneSucceeded()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Cap dt to prevent huge jumps
        if (dt > 0.1f) {
            dt = 0.1f;
        }

        // Process input
        renderer.processInput();

        // Update swarm with fixed timestep (full speed now!)
        swarm.update(targetDt); // Run at normal speed - faster training!

        // Check if any drone succeeded right after update (don't wait for next iteration!)
        if (swarm.hasAnyDroneSucceeded()) {
            break; // Exit immediately when one drone finds the hole!
        }

        // Render
        renderer.render(swarm);

        // Sleep to maintain ~60 FPS and make simulation visible
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // FPS counter and status display
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            float episodeTime = swarm.getEpisodeTime();
            float maxTime = swarm.getMaxEpisodeTime();
            int timePercent = (int)((episodeTime / maxTime) * 100.0f);

            std::cout << "Поколение: " << std::setw(4) << swarm.getGeneration()
                      << " | Время: " << std::fixed << std::setprecision(1) << std::setw(4) << episodeTime
                      << "с/" << std::setw(4) << maxTime << "с (" << std::setw(3) << timePercent << "%)"
                      << " | Лучший результат: " << std::setw(6) << std::setprecision(0) << swarm.getBestFitness()
                      << " | FPS: " << frameCount
                      << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;

            // Auto-save every 10 generations
            if (swarm.getGeneration() % 10 == 0 && swarm.getGeneration() > 0) {
                swarm.saveBestNetwork(networkFile);
                std::cout << "  [Автосохранение лучшей нейросети]" << std::endl;
            }
        }
    }

    // If a drone succeeded, show victory screen for a few seconds
    if (swarm.hasAnyDroneSucceeded()) {
        std::cout << "\n🎉🎉🎉 УСПЕХ! ДРОН ПРОШЁЛ ЧЕРЕЗ ДЫРУ! 🎉🎉🎉" << std::endl;
        std::cout << "Показываю результат 5 секунд..." << std::endl;

        // Change window title to show success!
        renderer.setWindowTitle("🎉🎉🎉 УСПЕХ! ДРОН НАШЁЛ ДЫРУ! 🎉🎉🎉");

        // Keep rendering for 5 seconds to see the success
        for (int i = 0; i < 5 * 60; i++) { // 5 seconds at 60 FPS
            renderer.render(swarm);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    // Save network before exit
    std::cout << "\nСохранение лучшей нейросети..." << std::endl;
    swarm.saveBestNetwork(networkFile);

    std::cout << "Симуляция завершена. Финальное поколение: " << swarm.getGeneration() << std::endl;
    std::cout << "Лучший результат: " << swarm.getBestFitness() << std::endl;

    return 0;
}
