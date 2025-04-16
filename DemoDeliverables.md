# TODO: Features
### In Progress
- [ ] Robust Model Buffers
- [ ] Better Model Data Management
- [ ] Tonemapping Pass (Compute?)
- [ ] Better Transform matrix calculation when using a camera 
- [ ] Compute FXAA
- [x] Manage command buffers/resources external to the stages, and move pipeline creation logic into the Stage setup
- [ ] ImGUI Implementation
- [x] Recycle Resources
### V1.0
- [ ] Metallic/Specular PBR Shading
- [ ] Occlusion Culling
- [ ] Shadow
- [ ] MultiThreaded Rendering
- [ ] Scene Hierarchy
### Backlog
- [ ] Scripting
- [ ] Scene Editor
- [ ] Model Buffer Based on Pipeline
- [ ] Compute workload dispatchable from Engine
- [ ] Physics (Library)
- [ ] Visualize Frustum

# ROUGH IDEAS
* Compute Generating Vertex/Normal/UV/Indices culled buffers?
* Different AA (SMAA/HBAO+...)
* GPU Particle System
* Procedural Generation
* Audio
* More Modern Rendering Features

# MISC Thoughts
* Compute Async, more early tasks while DCG buffer is generated

* The Update Loop will remain controlled by a BVK file, however having the engine have access to the renderer to send information such as model data, loading models, requesting pipelines & maybe more just makes the most sense. Maybe this means some logic could be abstracted from the renderer, and pushing into the engine. I want to be VERY CAREFUL with this, because I have already experienced the complexity of a RenderGraph. My idea is that a healthy combination of hard coded & dynamic passes would work, kind of a sudo render graph implementation that would have a chance to avoid a lot of unnecessary complexities by having more assumptions being able to be made. 

* I fear that buffer management will become complicated. A potential solution could be pre-loading the models to know exactly what size the buffer needs to be however that feels like a lame solution. Maybe someday I feel brave I will attempt a proper buffer manager that would auto allocate more buffers. Maybe the render loop could be split into vertex (and other) buffer calls. This would mean that each pipeline gets it's own vertex buffer and if the buffer grows too big, it would get assigned another. I would like to know a solution for calculating a proper buffer size, and I should look into the proper VMA flags for the best performance.

* I need a solution for loading models, specifically models with multiple mesh. Currently, when I uplaod individual mesh it auto assigns the vertex offset. My fear with multiple mesh loads in one model (Which would combine all model data into 1 upload) is that the offset for the buffer is not properly applied to each mesh. I think this could be solved with a mesh class holding it's own respective model data. This could still be copied with 1 call from the staging buffer by first calculating the total size of all of the mesh (Could be useful to keep track while loading the model). Then we process each individual mesh updating the offsets as we go along. I would like some correlation between mesh & shader set/material. I think having a hashmap with each loaded material so that the mesh contains the index of it's material.

* I might consider an ECS-ish system, maybe having the data that is sent to the renderer be pre-packed and updated in their respective "homes" would make sense. Currently moving the required data to the renderer makes a bunch of copies. I would rather send pointers to the buffers. This could cause issues if multi-threading (but also, how am I supposed to multi-thread the Renderer seperate from the Engine when data is being constantly update and interacting with the GPU side of things)

## ALIASING: 
* FXAA: It takes the current rendered Image & outputs a new image. We should only need 2 image resources per frame for most if not all rendering and post passes. Main Render->IMG[0]->FXAA->IMG[1]->TONEMAPPING->[0]->SWAPCHAIN(Need solution for rendering directly to swapchain on last step of rendering, but I think that should be trivial)