# Computer Graphics Studies

## Overview

This project collects experiments and implementations related to real-time computer graphics. The code is organized as a cross‑platform engine that builds on Windows and Linux using CMake. Third‑party libraries such as Volk and FLTK are included via Git submodules.

## Prerequisites

* **Linux**: clang/clang++ and [ninja](https://ninja-build.org)
* **Windows**: Visual Studio C++ tools and [ninja](https://ninja-build.org)
* CMake 3.12 or newer

## Initializing Submodules

Before building, fetch the external dependencies:

* **Linux**: `./init.sh`
* **Windows**: `init.bat`

These scripts install required packages on Linux and run `git submodule update --init --recursive`.

## Building with CMake

Example configuration and build commands using the provided presets:

```bash
# Debug build on Linux
cmake --preset clang-debug
cmake --build --preset clang-debug

# Release build on Linux
cmake --preset clang-release
cmake --build --preset clang-release
```

On Windows, replace the preset names with `x64-debug` or `x64-release`.
