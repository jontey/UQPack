#!/bin/bash

# Check if emscripten is available
if ! command -v emcmake &> /dev/null; then
    echo "Error: emcmake not found. Please install and activate Emscripten first."
    echo "Visit https://emscripten.org/docs/getting_started/downloads.html for installation instructions."
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build_wasm
cd build_wasm

# Configure with CMake using Emscripten
emcmake cmake .. \
    -DBUILD_EXECUTABLE=OFF \
    -DBUILD_SHARED_LIBS=OFF

# Build
cmake --build . -j

# # Install to dist directory
mkdir -p ../dist
cmake --install . --prefix ../dist
