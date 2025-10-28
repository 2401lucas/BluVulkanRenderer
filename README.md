# BluVulkan Renderer

A Vulkan rendering engine built from the ground up in C++.

## Overview

BluVulkanRenderer is a custom rendering engine using Vulkan 1.3 to achieve maximum performance and control over the GPU pipeline

<img width="400" height="200" alt="image" src="https://github.com/user-attachments/assets/17d6f67b-3c75-4c5e-8b4a-938154ac6f4c" />
<img width="400" height="200" alt="image" src="https://github.com/user-attachments/assets/1038285e-834f-4a01-a9a3-c3aa408bd287" />

# Key Features
## Advanced Rendering Pipeline
- Physically Based Rendering (PBR) with full material support including metalness, roughness, normal mapping, ambient occlusion, and emission
- Multi-stage rendering architecture with modular pass system for flexible pipeline configuration
- Dynamic rendering utilizing Vulkan 1.3's modern rendering features for reduced overhead
- Configurable draw modes supporting shaded, unlit, and wireframe visualization

## Performance Optimization
- GPU-driven rendering with compute-based frustum culling to minimize CPU overhead
- Indirect drawing using vkCmdDrawIndexedIndirect for efficient batch rendering
- Buffer Device Address (BDA) for bindless resource access, eliminating descriptor set bottlenecks
- Timeline semaphores for fine-grained GPU synchronization and parallel pass execution

## Culling & Visibility
- Frustum culling implemented entirely on GPU using compute shaders
- Bounding sphere pre-computation for efficient visibility testing
- Occlusion culling groundwork (Hi-Z buffer based) for additional performance gains
- Per-model instance data with efficient GPU-side command buffer generation

## Post-Processing & Anti-Aliasing
- FXAA (Fast Approximate Anti-Aliasing) implementation for edge smoothing
- Configurable post-processing pipeline with multiple output targets for debugging
- Multi-stage composition supporting UI overlay and effect blending
- Realtime screen space ambient occlusion

## Material & Texture System
- Bindless texturing
- Flexible material system with per-model texture assignments
- Texture loading and caching with deduplication
- Anisotropic filtering and mipmap generation for high-quality sampling

## Memory Management
- Vulkan Memory Allocator (VMA) integration for efficient GPU memory management
- Staged resource uploads with dedicated transfer queue utilization
- Persistent mapped buffers for low-latency CPU-to-GPU data transfer

## External Tool Experience
- ImGui integration for real-time parameter tuning and debugging
- RenderDoc, NSight, and PIX compatibility with labeled command buffers and resources
- Comprehensive debug visualization including render stage outputs and resource utilization
- Hot-swappable render settings allowing runtime configuration of culling, AA, and draw modes


## Architecture Highlights
- Modern C++ with EASTL for optimized standard library performance
- Separation of concerns between core rendering, scene management, and application logic
- Extensible stage system allowing easy addition of new rendering passes
- Input System with rebindable keys

## Demonstrated Skills
- Low-level graphics programming with direct GPU control
- Modern Vulkan API expertise including Vulkan 1.3 features
- GPU optimization techniques (culling, indirect rendering, bindless resources)
- Synchronization primitives (timeline semaphores, pipeline barriers)
- Memory management at both CPU and GPU levels
- Shader programming (GLSL compute and graphics shaders)
- Rendering algorithms (PBR, FXAA, culling techniques)
- Software architecture for extensible, maintainable graphics systems
- Performance profiling using industry-standard tools
