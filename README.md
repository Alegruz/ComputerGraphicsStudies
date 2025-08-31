# 🛠️ ComputerGraphicsStudies [WIP]

![status](https://img.shields.io/badge/status-WIP-orange)
![focus](https://img.shields.io/badge/focus-RealTimeGlobalIllumination-blue)

## Development philosophy of the `develop` branch

This branch is intended to develop the renderer with a focus on implementation first, then design, rather than design first, then implement. The reason is that I don't even know what the outcome would be, so it would be highly beneficial for me to abstract and design later once I get the grasp of the working code.

Target platforms (sorted by priority):

* CPU
  * Intel
  * AMD
* GPU
  * NVIDIA
  * AMD
  * Intel
  * Qualcomm
  * ARM
* OS
  * Windows
  * Linux
  * macOS
* Graphics API
  * Vulkan
  * Direct3D 12
  * Metal

## Development Log

* **[2025.08.18]**: base code with the main entry function per platforms.
* **[2025.08.19]**: base window code
* **[2025.08.20]**: command line parser draft
* **[2025.08.21]**: simple rasterization with a simple vertex animation in Win32
* **[2025.08.22]**: simple rasterization with a simple vertex animation in Linux
* **[2025.08.23]**: added vertex buffer and geometry classes
* **[2025.08.24]**: added cornell box scene, implemented multithreaded tiled rendering and frame buffering
* **[2025.08.25]**: sick day
* **[2025.08.26]**: cross platform threading support
* **[2025.08.27]**: fixed cross platform compilation issue
* **[2025.08.28]**: started Direct3D 12 support
* **[2025.08.29]**: added command line arguments for width and height
* **[2028.08.30]**: improved multithreads to worker threads instead of creating and deleting threads every frame