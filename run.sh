#!/bin/bash

# Build if needed
if [ ! -f "build/nndrons" ]; then
    echo "Building project..."
    mkdir -p build
    cd build
    cmake ..
    make -j4
    cd ..
fi

# Run the program
echo "Starting NN Drones simulation..."
./build/nndrons "$@"
