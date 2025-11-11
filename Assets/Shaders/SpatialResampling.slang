#include "Common.slang"

[shader("raygeneration")]
void RayGenMain()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    const uint2 screenSize = DispatchRaysDimensions().xy;
    const uint bufferIndexByPixel = pixel.y * screenSize.x + pixel.x;

    const uint spatialResampleRadius = (uint)ceil(min(screenSize.x, screenSize.y) * SPATIAL_NEIGHBORHOOD_RADIUS_RATE);

    const float4 gBufferData = gGBuffers[pixel];
    const float2 barycentrics = gBufferData.xy;
    const uint primitiveIndex = (uint)gBufferData.z;

    float3 sceneLinearColor = float3(0.0f, 0.0f, 0.0f);
    if (gBufferData.z >= 0.0f)
    {
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

        float3 emissiveColor = float3(0.0f, 0.0f, 0.0f);
        if (IsEmissive(primitiveIndex) == 1)
        {
            const float tempEmissiveStrength = 40.0f;
            emissiveColor = gColors[primitiveIndex] * tempEmissiveStrength;
        }

        float3 monteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

        float3 neeMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

        const float3 brdf = CalculateLambertianDiffuseBrdf(primitiveIndex);
        float3 parallelogramAreaLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
        if (IsEmissive(primitiveIndex) == 0)
        {
            // Spatial
#if SPATIAL_NEIGHBORS_COUNT > 0
            for (uint resampleIndex = 0; resampleIndex < SPATIAL_NEIGHBORS_COUNT; ++resampleIndex)
            {
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_SAME_PIXEL
                const int spatialNeighborX = 0;
                const int spatialNeighborY = 0;
#elif DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_PIXEL_BELOW
                const int spatialNeighborX = 0;
                const int spatialNeighborY = -1;
#endif
#else // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, FrameIndex + resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, FrameIndex + resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#else // NOT SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#endif // NOT SAMPLE_USING_FRAME_INDEX
#endif // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
                const uint2 spatialNeighborPixel = (uint2)clamp((int2)pixel + int2(spatialNeighborX, spatialNeighborY), int2(0, 0), int2(screenSize) - int2(1, 1));
                const uint spatialNeighborBufferIndex = spatialNeighborPixel.y * screenSize.x + spatialNeighborPixel.x;

                const float4 neighborGBufferData = gGBuffers[spatialNeighborPixel];
                if (neighborGBufferData.z < 0.0f)
                {
                    continue;
                }
                const uint samplesCount = gPrevParallelogramAreaLightReservoir[spatialNeighborBufferIndex].SamplesCount;

                if (samplesCount == 0)
                {
                    continue;
                }

                const float3 selectedProposalPosition = gPrevParallelogramAreaLightReservoir[spatialNeighborBufferIndex].ParallelogramAreaLightPositionSample;
                float3 parallelogramAreaLightPosition = selectedProposalPosition;
                float3 worldSampleDir = normalize(parallelogramAreaLightPosition - position);
                if(dot(worldSampleDir, normal) <= 0.0f)
                {
                    continue;
                }

                ParallelogramAreaLightReservoir reservoir = (ParallelogramAreaLightReservoir)0;

                CombineParallelogramAreaLightReservoirs(reservoir, pixel, spatialNeighborPixel, bufferIndexByPixel, spatialNeighborBufferIndex, position, normal);

                gParallelogramAreaLightReservoir[bufferIndexByPixel] = reservoir;
            }
#endif // SPATIAL_NEIGHBORS_COUNT > 0

            const float3 selectedProposalPosition = gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightPositionSample;
            const float3 selectedProposalNormal = gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightNormalSample;
            const float pdf = gParallelogramAreaLightReservoir[bufferIndexByPixel].Pdf;
            const float weight = gParallelogramAreaLightReservoir[bufferIndexByPixel].Weight;

            const float3 parallelogramAreaLightPosition = selectedProposalPosition;
            const float3 hitPositionToPointLightPosition = parallelogramAreaLightPosition - position;
            const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
            const float distance = sqrt(distanceSquared);
            const float3 worldSampleDir = normalize(parallelogramAreaLightPosition - position);

            if (dot(worldSampleDir, normal) > 0.0f)
            {
                // Trace a ray in the sampled direction.
                RayDesc ray;
                ray.Origin = position;
                ray.Direction = worldSampleDir;
                ray.TMin = 0.001;
                ray.TMax = 10000.0;

                RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, 0 };
                TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

                if (IsEmissive(newPayload.PrimitiveIndex) == 1)
                {
                    const float3 incomingRadiance = newPayload.Color.rgb;
                    const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
                    const float cosAtLight = max(0.0f, dot(-worldSampleDir, selectedProposalNormal));
                    const float geometryTerm = cosAtSurface * cosAtLight / distanceSquared;

                    if (cosAtLight > 0.0f)
                    {
                        const float pdfInSolidAngle = (distanceSquared / cosAtLight) * pdf;
                        const float hemisphereCosineWeightedPdf = cosAtSurface / PI;
                        const float misWeight = PowerHeuristic(PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, pdfInSolidAngle, POINT_LIGHT_SAMPLES_COUNT, 0.0f, HEMISPHERE_SAMPLES_COUNT, hemisphereCosineWeightedPdf);
                        const float3 integrand = incomingRadiance * geometryTerm * misWeight * weight;
                        parallelogramAreaLightMonteCarloEstimate = integrand * brdf;
                    }
                }
            }
        }
#endif
        neeMonteCarloEstimate += parallelogramAreaLightMonteCarloEstimate;

        float3 pointLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if POINT_LIGHT_SAMPLES_COUNT > 0
        {
        // Spatial
#if SPATIAL_NEIGHBORS_COUNT > 0
            for (uint resampleIndex = 0; resampleIndex < SPATIAL_NEIGHBORS_COUNT; ++resampleIndex)
            {
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_SAME_PIXEL
                const int spatialNeighborX = 0;
                const int spatialNeighborY = 0;
#elif DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_PIXEL_BELOW
                const int spatialNeighborX = 0;
                const int spatialNeighborY = -1;
#endif
#else // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, FrameIndex + resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, FrameIndex + resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#else   // NOT SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#endif  // NOT SAMPLE_USING_FRAME_INDEX
#endif  // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
                const uint2 spatialNeighborPixel = (uint2)clamp((int2)pixel + int2(spatialNeighborX, spatialNeighborY), int2(0, 0), int2(screenSize) - int2(1, 1));
                const uint spatialNeighborBufferIndex = spatialNeighborPixel.y * screenSize.x + spatialNeighborPixel.x;

                const float4 neighborGBufferData = gGBuffers[spatialNeighborPixel];
                if (neighborGBufferData.z < 0.0f)
                {
                    continue;
                }
                const uint samplesCount = gPrevPointLightReservoir[spatialNeighborBufferIndex].SamplesCount;

                if (samplesCount == 0)
                {
                    continue;
                }

                PointLightReservoir reservoir = (PointLightReservoir)0;

                CombinePointLightReservoirs(reservoir, pixel, spatialNeighborPixel, bufferIndexByPixel, spatialNeighborBufferIndex, position, normal);

                gPointLightReservoir[bufferIndexByPixel] = reservoir;
            }
#endif  // SPATIAL_NEIGHBORS_COUNT > 0

            const uint selectedProposalIndex = gPointLightReservoir[bufferIndexByPixel].LightIndexSample;
            const float pdf = gPointLightReservoir[bufferIndexByPixel].Pdf;
            const float weight = gPointLightReservoir[bufferIndexByPixel].Weight;
            const float3 pointLightPosition = gPointLightInfos[selectedProposalIndex].Positions;
            const float3 hitPositionToPointLightPosition = pointLightPosition - position;
            const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
            const float distance = sqrt(distanceSquared);
            const float3 worldSampleDir = normalize(pointLightPosition - position);

            if (dot(worldSampleDir, normal) > 0.0f)
            {
                // Trace a ray in the sampled direction.
                RayDesc ray;
                ray.Origin = position;
                ray.Direction = worldSampleDir;
                ray.TMin = 0.001;
                ray.TMax = 10000.0;

                RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, 0 };
                TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

                if (newPayload.RayT > distance)
                {
                    const float pointLightStrength = 1000.0f;
                    const float3 incomingRadiance = gPointLightInfos[selectedProposalIndex].Color * pointLightStrength;
                    const float geometryTerm = dot(worldSampleDir, normal) / distanceSquared;

                    // These are 0 because no parallelogram area light sampling nor hemisphere sampling can sample point lights
                    float parallelogramAreaLightPdf = 0.0f;
                    float hemisphereSamplePdf = 0.0f;
                    const float misWeight = PowerHeuristic(POINT_LIGHT_SAMPLES_COUNT, pdf, HEMISPHERE_SAMPLES_COUNT, hemisphereSamplePdf, PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, parallelogramAreaLightPdf);
                    const float3 integrand = incomingRadiance * geometryTerm * misWeight * weight;

                    pointLightMonteCarloEstimate = integrand * brdf;
                }
            }
        }
#endif
        neeMonteCarloEstimate += pointLightMonteCarloEstimate;

        float3 indirectIlluminationMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if HEMISPHERE_SAMPLES_COUNT > 0
        {
            // Spatial
#if SPATIAL_NEIGHBORS_COUNT > 0
            for (uint resampleIndex = 0; resampleIndex < SPATIAL_NEIGHBORS_COUNT; ++resampleIndex)
            {
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_SAME_PIXEL
                const int spatialNeighborX = 0;
                const int spatialNeighborY = 0;
#elif DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_METHOD == DEBUG_SPATIAL_RESAMPLING_NEIGHBORS_PIXEL_BELOW
                const int spatialNeighborX = 0;
                const int spatialNeighborY = -1;
#endif
#else // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
#if SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, FrameIndex + resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, FrameIndex + resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#else  // NOT SAMPLE_USING_FRAME_INDEX
                const int spatialNeighborX = (int)RandomInt(pixel, resampleIndex * 7, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
                const int spatialNeighborY = (int)RandomInt(pixel, resampleIndex * 11, 0, spatialResampleRadius * 2 + 1) - (int)spatialResampleRadius;
#endif  // NOT SAMPLE_USING_FRAME_INDEX
#endif // NOT DEBUG_SPATIAL_RESAMPLING_NEIGHBORS
                const uint2 spatialNeighborPixel = (uint2)clamp((int2)pixel + int2(spatialNeighborX, spatialNeighborY), int2(0, 0), int2(screenSize) - int2(1, 1));

                const uint spatialNeighborBufferIndex = spatialNeighborPixel.y * screenSize.x + spatialNeighborPixel.x;

                const float4 neighborGBufferData = gGBuffers[spatialNeighborPixel];
                if (neighborGBufferData.z < 0.0f)
                {
                    continue;
                }

                const uint samplesCount = gPrevIndirectLightReservoir[spatialNeighborBufferIndex].SamplesCount;

                if (samplesCount == 0)
                {
                    continue;
                }

                IndirectLightReservoir reservoir = (IndirectLightReservoir)0;

                CombineIndirectLightReservoirs(reservoir, pixel, spatialNeighborPixel, bufferIndexByPixel, spatialNeighborBufferIndex, position, normal);

                gIndirectLightReservoir[bufferIndexByPixel] = reservoir;
            }
#endif // SPATIAL_NEIGHBORS_COUNT > 0

            const float weight = gIndirectLightReservoir[bufferIndexByPixel].Weight;
            const float3 worldSampleDir = gIndirectLightReservoir[bufferIndexByPixel].Direction;
            const float pdf = gIndirectLightReservoir[bufferIndexByPixel].Pdf;

            if (dot(worldSampleDir, normal) > 0.0f)
            {
                // Trace a ray in the sampled direction.
                RayDesc ray;
                ray.Origin = position;
                ray.Direction = worldSampleDir;
                ray.TMin = 0.001;
                ray.TMax = 10000.0;

                RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, 0 };
                TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

                // 3. Incoming radiance
                const float3 incomingRadiance = newPayload.Color.rgb;

                // 4. Cosine term is embedded in the sampling pdf for cosine-weighted hemisphere sampling.

                // 5. Integrand
                const float pointLightPdf = 0.0f; // assume that the sampled direction doesn't hit a point light
                float parallelogramAreaLightPdf = 0.0f;
                if (newPayload.PrimitiveIndex != 0xFFFFFFFF && IsEmissive(newPayload.PrimitiveIndex) == 1)
                {
                    const ParallelogramAreaLightInfo lightInfo = gParallelogramAreaLightInfos[0];
                    const float3 e1 = lightInfo.Positions[1] - lightInfo.Positions[0];
                    const float3 e2 = lightInfo.Positions[2] - lightInfo.Positions[0];
                    const float parallelogramArea = length(cross(e1, e2));
                    parallelogramAreaLightPdf = 1.0f / parallelogramArea;
                }

                const float misWeight = PowerHeuristic(HEMISPHERE_SAMPLES_COUNT, pdf, POINT_LIGHT_SAMPLES_COUNT, pointLightPdf, PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, parallelogramAreaLightPdf);
                const float3 f = brdf * incomingRadiance;
                indirectIlluminationMonteCarloEstimate = f * misWeight * weight;
            }
        }
#endif

        monteCarloEstimate += neeMonteCarloEstimate;
        monteCarloEstimate += indirectIlluminationMonteCarloEstimate;

        // Write the raytraced color to the output texture.
        sceneLinearColor = monteCarloEstimate.rgb + emissiveColor.rgb;
    }

    // 1) fireflies/NaNs guard (optional but wise for path tracing)
    // sceneLinearColor = max(sceneLinearColor, float3(0.0, 0.0, 0.0)); // avoid NaN and negative values
    // sceneLinearColor = min(sceneLinearColor, float3(1e6, 1e6, 1e6)); // avoid INF

    // 2) exposure (tweak per scene/camera)
    sceneLinearColor = ApplyExposure(sceneLinearColor, /*exposureEV*/ 0.0);

    // 3) tonemap (choose one)
    const float3 tonemapped = TonemapACES(sceneLinearColor);

    // 4) sRGB encode for UNORM output
    const float3 srgb = LinearToSRGB(tonemapped);

    // 5) write
    const float4 finalColor = float4(srgb, 1.0f);
    gRenderTarget[pixel] = finalColor;
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