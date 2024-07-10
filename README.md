# BluRenderer - A Vulkan Renderer (WIP)
[Read more about my project here](https://2401-lucas-github-io.vercel.app/)

![image](https://github.com/2401lucas/BluVulkanRenderer/assets/32739337/02cc82d1-0f9f-42bb-b2e6-63f9150d4069)

### Current & Future Features
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
- [x] Compute Shader SSAO w/ Depth Prepass
- [x] Debug Shader Views
- [ ] Multi-threaded Command Buffer Recording
- [ ] Model class Refactor for GPU Instancing
- [ ] Compute Culling (Frustrum & Occlusion)
- [ ] HBAO+
- [ ] Temporal Anti-Aliasing (TAA)  
- [ ] Texture Streaming
- [ ] Forward Plus Rendering
- [ ] Animations
- [ ] GPU Particle Simulation & Lighting
- [ ] Compute Post Processing
- [ ] Clearer Descriptor Set Creation
- [ ] Global Illumination
- [ ] Tiled Rendering
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
