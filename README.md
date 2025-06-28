# 🛠️ ComputerGraphicsStudies – Vulkan-Based Real-Time Renderer [WIP]

![status](https://img.shields.io/badge/status-WIP-orange)
![focus](https://img.shields.io/badge/focus-RealTimeGlobalIllumination-blue)

This repository hosts an experimental Vulkan-based real-time renderer that I’m building from scratch to explore physically-based rendering techniques—especially global illumination methods like ReSTIR—in a production-aware context.

The goal is to test the feasibility of GPU-friendly, scalable global illumination under constraints found in modern game engines, while building a minimal, modular system for experimentation.

> ⚠️ **This project is a work in progress.** It is not feature-complete and is under active development in my personal time.

---

## Overview

This project collects experiments and implementations related to real-time computer graphics. The code is organized as a cross‑platform engine that builds on Windows and Linux using CMake. Third‑party libraries such as Volk and FLTK are included via Git submodules.

---

## Prerequisites

- **Linux**: clang/clang++ and [ninja](https://ninja-build.org)
- **Windows**: Visual Studio C++ tools and [ninja](https://ninja-build.org)
- CMake 3.12 or newer

---

## Initializing Submodules

Before building, fetch the external dependencies:

- **Linux**: `./init.sh`
- **Windows**: `init.bat`

These scripts install required packages on Linux and run:

```bash
git submodule update --init --recursive
```

## Building with CMake

Example configuration and build commands using the provided presets:

```
# Debug build on Linux
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build on Linux
cmake --preset clang-release
cmake --build --preset clang-release
```

On Windows, use the x64 presets:

```
# Debug build on Windows
cmake --preset x64-debug
cmake --build --preset x64-debug

# Release build on Windows
cmake --preset x64-release
cmake --build --preset x64-release
```

## Running the Sample Applications

After building, executables are placed in out/build/<preset>. Run them directly from that directory:

```
out/build/x64-debug/LauncherApp.exe   # Launches the engine with default config
```

Replace x64-debug with x64-release to run the release binaries.

## Author

[Minha Armada Ju (Alegruz)](https://alegruz.github.io/)<br>
📧 alegruz@khu.ac.kr | [GitHub](https://github.com/Alegruz) | [LinkedIn](https://www.linkedin.com/in/alegruz)