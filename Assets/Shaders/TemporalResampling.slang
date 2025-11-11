#include "Common.slang"

[shader("raygeneration")]
void RayGenMain()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    const uint2 screenSize = DispatchRaysDimensions().xy;
    const uint bufferIndexByPixel = pixel.y * screenSize.x + pixel.x;

    const float4 gBufferData = gGBuffers[pixel];
    const float2 barycentrics = gBufferData.xy;
    const uint primitiveIndex = (uint)gBufferData.z;

    if (gBufferData.z < 0.0f)
    {
        return;
    }

    // Get the base index of the triangle's first 16 bit index.
    const uint indexSizeInBytes = 2;
    const uint indicesPerTriangle = 3;
    const uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    const uint baseIndex = primitiveIndex * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);

    const float3 vertexPositions[3] =
        {
            gVertices[indices[0]].Position,
            gVertices[indices[1]].Position,
            gVertices[indices[2]].Position
        };

    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] =
        {
            gVertices[indices[0]].Normal,
            gVertices[indices[1]].Normal,
            gVertices[indices[2]].Normal
        };

    const float3 position = InterpolateByBarycentric(vertexPositions, barycentrics);
    const float3 normal = InterpolateByBarycentric(vertexNormals, barycentrics);
    
#if PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
    if(IsEmissive(primitiveIndex) == 0)
    {
        // Temporal reuse
        {
#if ENABLE_TEMPORAL_RESAMPLING
            ParallelogramAreaLightReservoir reservoir = (ParallelogramAreaLightReservoir)0;

            CombineParallelogramAreaLightReservoirs(reservoir, pixel, pixel, bufferIndexByPixel, bufferIndexByPixel, position, normal);

            gPrevParallelogramAreaLightReservoir[bufferIndexByPixel] = reservoir;
#else // NOT ENABLE_TEMPORAL_RESAMPLING
            gPrevParallelogramAreaLightReservoir[bufferIndexByPixel] = gParallelogramAreaLightReservoir[bufferIndexByPixel];
#endif // NOT ENABLE_TEMPORAL_RESAMPLING
        }
    }
#endif

#if POINT_LIGHT_SAMPLES_COUNT > 0
    {
        // Temporal
        {
#if ENABLE_TEMPORAL_RESAMPLING
            PointLightReservoir reservoir = (PointLightReservoir)0;

            CombinePointLightReservoirs(reservoir, pixel, pixel, bufferIndexByPixel, bufferIndexByPixel, position, normal);

            gPrevPointLightReservoir[bufferIndexByPixel] = reservoir;
#else  // NOT ENABLE_TEMPORAL_RESAMPLING
            gPrevPointLightReservoir[bufferIndexByPixel] = gPointLightReservoir[bufferIndexByPixel];
#endif // NOT ENABLE_TEMPORAL_RESAMPLING
        }
    }
#endif

#if HEMISPHERE_SAMPLES_COUNT > 0
    {
        // Temporal
        {
#if ENABLE_TEMPORAL_RESAMPLING
            IndirectLightReservoir reservoir = (IndirectLightReservoir)0;

            CombineIndirectLightReservoirs(reservoir, pixel, pixel, bufferIndexByPixel, bufferIndexByPixel, position, normal);

            gPrevIndirectLightReservoir[bufferIndexByPixel] = reservoir;
#else  // NOT ENABLE_TEMPORAL_RESAMPLING
            gPrevIndirectLightReservoir[bufferIndexByPixel] = gIndirectLightReservoir[bufferIndexByPixel];
#endif // NOT ENABLE_TEMPORAL_RESAMPLING
        }
    }
#endif
}


[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 color = gColors[PrimitiveIndex()];
    if (IsEmissive(PrimitiveIndex()) == 1)
    {
        const float tempEmissiveStrength = 40.0f;
        color *= tempEmissiveStrength;
    }
    payload.Color = float4(color, 1.0f);
    payload.RayT = RayTCurrent();
    payload.PrimitiveIndex = PrimitiveIndex();
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.0f, 0.0f, 1.0f);
    payload.Color = background;
    payload.RayT = FLT_MAX;
    payload.PrimitiveIndex = 0xFFFFFFFF;
}