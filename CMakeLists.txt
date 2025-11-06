cmake_minimum_required(VERSION 3.15)
project(NNDrons)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 3.3 REQUIRED)

# GLM (header-only)
find_package(glm REQUIRED)

# Eigen (header-only)
find_package(Eigen3 REQUIRED NO_MODULE)

# Source files
set(SOURCES
    src/main.cpp
    src/renderer.cpp
    src/environment.cpp
    src/drone.cpp
    src/neural_network.cpp
    src/rl_trainer.cpp
    src/swarm.cpp
)

# Header files
set(HEADERS
    include/renderer.h
    include/environment.h
    include/drone.h
    include/neural_network.h
    include/rl_trainer.h
    include/swarm.h
    include/vec3.h
)

add_executable(nndrons ${SOURCES} ${HEADERS})

target_include_directories(nndrons PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(nndrons
    OpenGL::GL
    glfw
    Eigen3::Eigen
)

# Platform specific settings
if(APPLE)
    target_link_libraries(nndrons "-framework Cocoa -framework IOKit")
endif()
