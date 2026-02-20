# Sprite Sheet Slicer
A simple, cross-platform tool for slicing sprite sheets into individual sprites.
Originally built for a custom game engine, now shared for all developers.

## Download
You can download the latest release here:
[https://miisan.itch.io/sprite-sheet-slicer](https://miisan.itch.io/sprite-sheet-slicer)

## Build & Run (Linux)

```bash
./run.sh
```

This script will automatically clone dependencies, build the project, and run it.

### Manual Build

```bash
# Clone ImGui if not present
git clone --depth 1 https://github.com/ocornut/imgui.git external/imgui

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./spritesheet_slicer
```

### Dependencies (Linux)

**Arch Linux:**
```bash
sudo pacman -S cmake gcc make git libgl
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake g++ make git libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Usage

Run the executable and load your sprite sheet to slice it into individual sprites.
Supports click selection, renaming, zooming, and grid overlay for precise slicing.
