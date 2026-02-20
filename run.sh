#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if imgui exists, clone if not
if [ ! -f "external/imgui/imgui.h" ]; then
    echo "Cloning ImGui..."
    rm -rf external/imgui
    git clone --depth 1 https://github.com/ocornut/imgui.git external/imgui
fi

# Build
mkdir -p build
cd build
cmake ..
make -j$(nproc)

# Run
echo ""
echo "Starting Sprite Sheet Slicer..."
./spritesheet_slicer
