# BluRenderer
What was originally supposed to be a "simple" vulkan renderer turned into a small 3D engine. All of the systems I have implemented are my design, or at least my own interpretation of others designs. I have purposely only followed the official Vulkan tutorial, and that was to get started. The rest I have figure out on my own. This forced me to really understand the systems I was implementing and means they probably aren't perfect, or at least have a lot of room for improvement. I am focused on fixing performance bottlenecks and implementing new features versus perfecting features I have already implemented.
 
## Entity Component System 
This was my first experience working with ECS, I have read about [Unity's DOTS implementation](https://unity.com/dots) and didn't appreciate it's design until I had a deep understanding of Game Engines and Renderers.
I opted to try and follow Unity's implementation loosely. Instead of grouping similar components together, entities with similar components are grouped together. This has a great benefit when itererating over entities as there's very little checking for the cpu to do and data is quite close together, greatly improving read speed from the ram to the cpu. One glaring issue for me was the lack of "one off" objects. For example, something simple like a door requiring it's own component doesn't seem right to me. I opted to add a script component, allowing for smaller, more niche logic to exist run without requiring an entire component and system to be added. It ended up being some sort of Hybrid-esque ECS but I am quite happy with it. It performs well and is intuitive to use.

## Memory Packing, Layouts and Leaks! 
Memory is not something I would've put much thought into before, but now it's second nature to organize, plan my memory layouts, usage and containers, especially for ecs as contiguous memory is key. I had to be clever to not have all of my memory stored in contiguous containers as if it grew to big and had to reallocate itself constantly it could get costly. For ecs this meant limiting my contiguous memory chunks to 16KB. A nice balance for cache efficiency.

## Cache Efficiency 
I found trying to program for high cache efficiency intriguing. There is nothing explicit about writing code with high cache efficiency besides high level architectural choices. I chose my design of ECS, where similiar entities are stored together with their components, as it made more sense to me than storing groups of components. I knew that I wanted to update all of an entities components together, and having them stored contiguously really helped cache read speeds. My best explanation of cache optimization would be similar to reading a book. When words are organized sequentially, left to right, for every page it's really easy to read. If words are spread out sporadically and you need to search for the next word, things slow down quickly. Another useful optimization, specifically for ECS components, keeping the data under 64 bytes allows for a single cache read to load the data in.

## Multiple Pipelines
In vulkan, each render pipeline can only support one pair of shaders. This means we will need multiple pipelines, and gives us reason to minimize the number of shader pairs we have. In my implementation mesh is sorted based on the pipeline that is being used. This allows for minimal binds BindVertexBuffer, BindIndexBuffer, BindPipeline, and BindDescriptorSet calls which can be an expensive operations.

```
bindBuffers();
for (auto& pipeline : pipelines) {
    bindPipeline();
    bindDescriptorSets();
    bindDescriptorSets();
    for (auto& mesh : pipelineMeshData) {
        updatePushConstants();
        drawIndexed();
    }
}
```

## Bindless Descriptors & The Multiple Pipeline Problem
While having multiple pipelines is great, it introduces a new issue. Too many pipelines. How many pipelines is too many? Well Unreal engine can use over 100000 pipelines, but an engine like ID Tech 7(Doom Eternal) has less than 500 pipelines. How they can manage this? ID Tech 7 uses a bindless design for their buffers, textures and materials. For Textures, texture arrays are uploaded to the Shader with the size being unbound. This requires [VK_EXT_descriptor_indexing](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_indexing.html) & [runtimeDescriptorArray](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceDescriptorIndexingFeatures.html), both Vulkan 1.2 Core Features). For Vertex/Index buffers this means having one large buffer that stores all the data.

## Mesh Shaders
What is a Mesh Shader? While Vertex & Fragment shaders are great they have a lot of limitations, some that require other shaders to be used in conjunction with them such as Hull, Tesselation, Domain & Geometry. This can be slow and are harder to optimize with their unpredictable nature. This is a problem as once mesh data is uploaded to the GPU, it should be static. There are exceptions(Such as skeletons) but updating large chunks of data on the gpu from the cpu is super slow and it cannot be done from a vertex shader. Compute Shaders are better designed for the hardware and can update GPU memory but are limited in what it can do, namely reading and writing from GPU memory without an output (Triangles for the Rasterization pipeline). This would still need a vertex shader. A Mesh Shader is both designed for the hardware, and have can output triangles. It can read a chunk of data, update it and output up to 256 triangles. It's main limitation (other than device compatibility) is from chunks being run independently and unable to communicate with other chunks. 

## Frustum Culling
Frustum culling is an optimization to minimize GPU drawcalls. In simple terms, this ensures that the only objects that are rendered are objects that are visible to the camera. [Inigo Quilez describes an optimized version of Frustum Culling](http://iquilezles.org/articles/frustumcorrect/) which helps catch some edgecases where certain objects could slip the the cracks and be drawn when not visible.

## The Vertex/Index Buffer
(TODO)

## In the works
Proper Scene Asset loading & a Scene Editor

ImGUI

Compute Pipeline

Multi-Threading(Asset Loading, Command Buffer Generation, Frustrum Culling/ECS update?)


## Resources
The majority of books I read about Graphics Programming ended up just being tutorials, showing the code and explaining what the code does instead of explaining the overall archetechture or the reasons for what they are programming. The best resource, and my most use resource by far was the Khronos documentation of vulkan. It is extremely detailed and contains all of the required information I needed with a complete explanation of the API. This allowed me to use whatever architecture I wanted. The hard part was coming up with an architecture, and I re-architectured this project a lot. While that may seem frustrating it allowed me to see the shortcomings of my previous designs. With that, I was able to understand the problems I was facing and how to fix them. It ended up giving me a much more in-depth understanding of the relationship of the Hardware, the Renderer and the Engine.

https://registry.khronos.org/vulkan/

https://vulkan-tutorial.com

https://www.gameenginebook.com

https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition

https://learnopengl.com

http://iquilezles.org/articles
