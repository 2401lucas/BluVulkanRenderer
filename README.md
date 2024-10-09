# BluRenderer - A Vulkan Renderer (WIP)
[Read more about my project here](https://2401-lucas-github-io.vercel.app/)

![image](https://github.com/user-attachments/assets/63469291-7188-43a8-beb7-3e16d88190cb)


### Current Features
- [x] Forward Rendering
- [x] Physically Based Rendering
- [x] Dynamic Lights (Directional, Point & Spot)
- [x] Static & Dynamic Models
- [x] glTF/glb Model Loading
- [x] Image Based Lighting
- [x] `ImGUI` Demo Interface
- [x] [FXAA from Nvidia White Paper](https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf) 
- [x] Fast Approximate Anti-Aliasing (FXAA)
- [x] Tonemapping HDR->SDR
- [x] Debug Shader Views
- [x] Fragment Shader SSAO

# Current WIP: Render Graph(Frame Graph)
At a high level a Render Graph is given each pass and their inputs/outputs which are used to generate frame specific resources, including automatically aliasing memory and generating barriers. This fixes a lot of design problems I encountered while working on this project. I was not a fan of having to micro manage every resource manually, and eventually adding features became a real chore to implement in  code. This system allows for much easier resource management and greatly improved debugging potential. It abstracts most, if not all direct graphics API out of the main renderer class. This makes it a lot easier to support other graphics API and leaves the main renderer class to be focused on the overall architecture, making it much more digestable to understand.
I also didn't like needing to manually create every resource and ended up with huge shaders with tons of debug cases, and having to ensure debug output was not modified by any following render pass. With my implementation of a RenderGraph, a new RenderGraph can be baked in realtime with a whole new core pipeline. This allows for debug specific pipelines, or switching between rendering techniques on the fly.

### Future Feature
- [ ] Multi-threaded Command Buffer Recording
- [ ] Clearer Descriptor Set Creation
- [ ] Model class Refactor for GPU Instancing
- [ ] Compute Culling (Frustrum & Occlusion)
- [ ] HBAO+
- [ ] Temporal Anti-Aliasing (TAA)  
- [ ] Texture Streaming
- [ ] Forward Plus Rendering
- [ ] Animations
- [ ] Compute Shader SSAO
- [ ] GPU Particle Simulation & Lighting
- [ ] Compute Post Processing
- [ ] Global Illumination
- [ ] Bloom
- [ ] God Rays
- [ ] Gizmos
- [ ] Scene Hierarchy view
- [ ] [Command Pool Refactor]() TODO: INSERT LINK TO BLOGPOST
- [ ] Atmospheric Scattering
- [ ] Light/Reflection Probes
- [ ] Rebake IBL with Static models Realtime
- [ ] Screen Space Reflections
- [ ] SMAA
- [ ] Compact Light GPU info
  
## Resources
### Tech
- RenderDoc
- NSight
- PIX

### Books & Documentation
https://registry.khronos.org/vulkan/

https://vulkan-tutorial.com

https://www.gameenginebook.com

https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition

https://www.realtimerendering.com/

https://learnopengl.com

http://iquilezles.org/articles
