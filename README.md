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


## Scene Resources Overview
### Lights
The light struct allows for the creation, deletion and modification of lights, unique to their type. When created or modified, the minimum required information for shaders are compacted in 4 vector4s, holding some combination of Color RBG(Intensity is premultiplied), LightType, Position XYZ, Direction XYZ, FOV, Light Falloffs(Constant, Linear & Quadratic). This also creates the light space matrix for shadow map calculations. This could be lowered to 3 vec4s if one float held all Light Falloffs at the cost of accuracy. Because all values are clamped between 0-1 this should be a non issue. 
### Models
Models are sepereated by Static and Dynamic. Static models are never moved, are baked into the IBL and their model data(Material Info & pre-multipled model matrices) are loaded onto the GPU once, only being updated if the scene changes(With an exception in the Demo UI to create & move static models). If a static model is loaded more than once it is automatically instanced, with each instance holding unique model matrix transform info. When drawing with `VkCmdDrawIndexed` gl_InstanceIndex is exposed in the vertex shader allowing for the model matrices to be indexed.

Dyanmic Models can be modified during runtime, with their model matrices being updated more frequently. They also support GPU instancing. 

### Material System
When a model is loaded, all of its material info(Colors, Textures...) are loaded into a material buffer which is then sent to the GPU in an SSBO and is indexed with a push constant. Currently, the material buffer is shared between models and is completely rewritten when adding or removing a single material. This could be changed to either a per object material buffer, or allocating a buffer with a **MAX_MATERIAL** limit allowing new materials to be added without reallocating the entire buffer. 
  
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
