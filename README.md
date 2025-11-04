# 🛠️ ComputerGraphicsStudies [WIP]

![status](https://img.shields.io/badge/status-Active-success)
![focus](https://img.shields.io/badge/focus-RealTimeGlobalIllumination-blue)
![platform](https://img.shields.io/badge/platform-Windows%20|%20Linux-lightgrey)
![api](https://img.shields.io/badge/API-Direct3D12-green)

## 🎯 Overview

This project implements **ReSTIR (Resampled Importance Sampling)** and **ReSTIR GI** in a custom-built Vulkan/D3D12 renderer.  
It aims to achieve **real-time multi-bounce global illumination** with *spatial and temporal resampling, reservoir sampling, and importance-driven light sampling* — all implemented from first principles.

---

## 🖼️ Visual Demo

<table>
<tr>
<td><img src="/Docs/2025_09_21.png" width="380"/></td>
<td><img src="/Docs/2025_09_26_indirect_light.png" width="380"/></td>
</tr>
<tr><td align="center">Temporal + Spatial Reuse</td><td align="center">3-Bounce Indirect Illumination</td></tr>
</table>

▶️ [YouTube Demo – Temporal Reuse (Sept 21, 2025)](https://youtu.be/T5tYhoAHzLU?si=QjRvoQsYwbGVvsPR)  
▶️ [YouTube Demo – Indirect Light (Sept 26, 2025)](https://youtu.be/OTc1I7WVSt0?si=H8-nFEEiRDQ9RRfz)

---


## ⚙️ Performance

| Configuration | GPU | Resolution | GI Bounces | Avg FPS |
|----------------|-----|-------------|-------------|----------|
| ReSTIR GI (Temporal + Spatial) | RTX 3060 Laptop | 1920×1080 | 3 | ~60 FPS |
| ReSTIR (Temporal Only) | RTX 3060 Laptop | 1920×1080 | 3 | ~72 FPS |

---

## 🔬 Techniques Implemented

- **Reservoir Sampling** (weighted random selection)
- **Resampled Importance Sampling (RIS)**
- **ReSTIR / ReSTIR GI** (temporal + spatial reuse)
- **Multiple Importance Sampling (MIS)**
- **Monte Carlo Integration**
- **Parallelogram Area Light Sampling**
- **Stratified Sampling (in progress)**
- **Möller–Trumbore Intersection**
- **PDF Stabilization**
- **Denoising Pipeline (planned)**
- **BSDF Evaluation (Lambertian, Emissive, in progress)**

---

## 📚 Write-Up

A detailed technical write-up covering:
- Mathematical derivation of ReSTIR
- Reservoir update & bias correction
- Implementation details in Vulkan & D3D12
- Debugging process and validation images

> 📝 *The write-up will be published soon on my personal site and linked here.*

---

## 🧩 Project Structure

* /Assets
  * shader files
* /Common
  * Common implementation shared across platforms
* /Core
  * Core subsystem such as math
* /CPU
  * CPU render device implementation
* /D3D12
  * Direct3D 12 render device implementation
* /Docs
  * Images
* /Linux
  * Linux application implementation
* /Win32
  * Win32 application implementation

---

## 🗂️ Repository Info

- **License:** MIT  
- **Language:** C++20 / Slang  
- **Status:** Active Development  
- **Platforms:** Windows, Linux  
- **APIs:** Vulkan, Direct3D 12  
- **Status:** Active Development  
- **Platforms:** Windows, Linux  
- **APIs:** Vulkan, Direct3D 12  
- **Build System:** CMake (cross-platform)  

---

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
* **[2025.08.30]**: improved multithreads to worker threads instead of creating and deleting threads every frame
* **[2025.08.31]**: fixed multithreaded rendering
* **[2025.09.01]**: implemented RGBA8 to BGRA8 conversion, adding d3d12 base code
* **[2025.09.02]**: added simple d3d12 objects initialization from dxgi objects and d3d12device to swap chain and back buffer render targets
* **[2025.09.03]**: implemented cornell box scene for d3d12
* **[2025.09.04]**: added d3d12 command allocator and command list, implemented simple rendering to back buffer. added per-frame synchronization.
* **[2025.09.05]**: fixed ref count issues when destroying d3d12 objects
* **[2025.09.06]**: added slang and d3d12 root signature and pipeline state object. **first rasterized image of cornell box!**
* **[2025.09.07]**: added acceleration structures draft
* **[2025.09.08]**: fixed some d3d12 debug layer warnings
* **[2025.09.09]**: implemented acceleration structure management and build process in renderer
* **[2025.09.10]**: added shader tables and uav output texture
* **[2025.09.11]**: added simple ray tracing shader and dispatch call
* **[2025.09.12]**: added SceneConstantBuffer for camera and emmisive geometry
* **[2025.09.13]**: fixed some D3D12 warnings
* **[2025.09.14]**: added descriptor heap class for managing descriptor heaps; refactored related resource allocation logic. First raytracing scene rendering.
* **[2025.09.15]**: fixed raytracing. **first raytraced image of cornell box!**
  * ![2025.09.15](/Docs/2025_09_15.png)
  * [Video demonstration on YouTube](https://youtu.be/LAAVhncfGXU?si=BirIZVkdHsCbQXmf)
* **[2025.09.16]**: skipped
* **[2025.09.17]**: added parallelogram area lights and point lights
  * ![2025.09.17](/Docs/2025_09_17.png)
  * [Video demonstration on YouTube](https://youtu.be/OKA5jCA3Jk8?si=HgZrp6UfRgSOrpID)
* **[2025.09.18]**: implemented monte carlo integration and multiple importance sampling of point lights and hemisphere sampling. **first global illumination image of cornell box!**
* **[2025.09.19]**: added parallelogram area light sampling into multiple importance sampling.
  * ![2025.09.19](/Docs/2025_09_19.png)
  * [Video demonstration on YouTube](https://youtu.be/7dzZOfVoNV8?si=yqgsiyu-zKcmHnZ7)
* **[2025.09.20]**: Implemented RIS (Resampled Importance Sampling) for parallelogram area light and point lights. Fixed some bugs in MIS.
  * ![2025.09.20](/Docs/2025_09_20.png)
  * [Video demonstration on YouTube](https://youtu.be/OTc1I7WVSt0?si=H8-nFEEiRDQ9RRfz)
* **[2025.09.21]**: Added reservoir and temporal resampling.
  * ![2025.09.21](/Docs/2025_09_21.png)
  * [Video demonstration on YouTube](https://youtu.be/T5tYhoAHzLU?si=QjRvoQsYwbGVvsPR)
    * Problems:
      * Noise patterns are visible
      * Point light sampling shows hiccups in certain frames (probably related to the hash function used)
      * If we keep accumulating the weights in a reservoir, this means that the comparison canonical sample < (current weight / weighted sum) will be biased towards the first few samples. This means that the reservoir will be dominated by the first few samples, which is not what we want. -> **fixed on 2025.09.22**
    * Next:
      * Spatial resampling
      * Better sampling function
    * Stratified sampling
* **[2025.09.22]**: Fixed some issues.
  * ![2025.09.22](/Docs/2025_09_22.png)
  * [Video demonstration on YouTube](https://youtu.be/0al30gyh4E8)
  * Fixes:
    * Reservoir weight accumulation is per-frame, not across frames. This fixes the bias issue where the first few samples dominate the reservoir.
    * Final integrand used suboptimal pdf for MIS/RIS weighting, not the desired pdf.
* **[2025.09.23]**: Fixed a bug where reservoirs were updated for bounced rays. Added spatial resampling draft.
  * Problems:
    * Spatial resampling causes spilling artifacts.
* **[2025.09.24]**: Fixed reservoir update where each reservoirs must be updated per pixel.
* **[2025.09.25]**: Fixed GPU resources debug name issue.
* **[2025.09.26]**: Fixing spatial resampling artifacts.
  * ![2025.09.26](/Docs/2025_09_26.png)
  * ![2025.09.26_parallelogram_area_light](/Docs/2025_09_26_parallelogram_area_light.png)
  * ![2025.09.26_point_light](/Docs/2025_09_26_point_light.png)
  * ![2025.09.26_indirect_light](/Docs/2025_09_26_indirect_light.png)
* **[2025.09.27]**: skipped
* **[2025.09.28]**: validating algorithms.
  * Problems:
    * Spatial resampling still cause problems such as spilling artifacts and sudden whitening of the scene.
* **[2025.09.29]**: Debugging.
  * Debugging Results:
    * The issue is not just spatial resampling. When I enable only temporal resampling, the results seems fine. If I enable only spatial resampling, the results has some issues, but it doesn't seem as bad as when both temporal and spatial resampling are enabled. This means that the issue is probably in the interaction between temporal and spatial resampling.