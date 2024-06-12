# BluRenderer - Vulkan
![image](https://github.com/2401lucas/BluVulkanRenderer/assets/32739337/1b532a7f-6f4a-4723-98e9-e8b315b7ffe8)


## Features
- PBR
- IBL w/ BRDFLUT & Irradiance cubemap
- Multi-threaded command buffer recording
- Double/Triple buffering
- GPU Compute Particles
- MSAA & FXAA
- ImGUI
- Tone mapping

## Rendering Pipeline
### Forward Renderer
- Render Skybox
- Render Models
- AA
- UI

### An Optimized AA Integration
Having AA integrated into an existing post processing pass can increase performance. As an example, in a single full screen pass: FXAA + composite bloom results + color grading. This could also allow AA to only be applied where nessicary, such as areas with a strong motion blur or Depth of Field


## Features to implement
- FXAA (In Progress)
- SMAA
- Culling
- More Post Processing passes
- More UI Features and information 
- Shadows / Baking Shadows
- SSAO/GTAO/HBAO
- glTF scene loading
  
## Resources
### Tech
- RenderDoc
- NSight
- PIX

### Books
The majority of books I read about Graphics Programming ended up just being tutorials, showing the code and explaining what the code does instead of explaining the overall archetechture or the reasons for what they are programming. The best resource, and my most use resource by far was the Khronos documentation of vulkan. It is extremely detailed and contains all of the required information I needed with a complete explanation of the API. 

https://registry.khronos.org/vulkan/

https://vulkan-tutorial.com

https://www.gameenginebook.com

https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition

https://learnopengl.com

http://iquilezles.org/articles
