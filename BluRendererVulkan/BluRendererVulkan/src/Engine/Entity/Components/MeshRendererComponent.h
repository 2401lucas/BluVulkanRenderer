#pragma once
#include "BaseComponent.h"
#include "../../../Render/Math/MathUtils.h"
#include "../../../Render/Buffer/BufferAllocator.h"

struct MeshRenderer : BaseComponent {
    MemoryChunk vertexMemChunk;
    MemoryChunk indexMemChunk;
    uint32_t indexCount;
    BoundingBox boundingBox;
};