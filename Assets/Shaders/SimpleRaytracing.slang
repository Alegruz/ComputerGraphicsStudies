// shaders.slang

#include "Common.slang"

void
UpdatePointLightReservoir(out uint outLightIndexSample, out float outDesiredPdf, inout float outWeightsSum, inout uint outSamplesCount, const float weight, const float desiredPdf, const uint2 pixel, const uint sampleIndex, const uint proposalIndex, const uint bufferIndexByPixel)
{
    outWeightsSum += weight;
    outSamplesCount += 1;
#if SAMPLE_USING_FRAME_INDEX
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + sampleIndex * RESAMPLE_COUNT + proposalIndex, 4);
#else  // NOT SAMPLE_USING_FRAME_INDEX
    const float canonicalSample = CanonicalSample(pixel, sampleIndex * RESAMPLE_COUNT + proposalIndex, 4);
#endif // NOT SAMPLE_USING_FRAME_INDEX
    const float probability = weight / outWeightsSum;
    if (canonicalSample < probability)
    {
        outLightIndexSample = proposalIndex;
        outDesiredPdf = desiredPdf;
    }
}

float
RisPointLight(out uint outLightIndexSample, out float outDesiredPdf, inout float outWeightsSum, inout uint outSamplesCount, const uint2 pixel, const uint sampleIndex, const float3 hitPosition, const float3 normal, const uint bufferIndexByPixel)
{
    for (uint proposalIndex = 0; proposalIndex < RESAMPLE_COUNT; ++proposalIndex)
    {
        // generate x_i ~ p
#if SAMPLE_USING_FRAME_INDEX
        const uint pointLightIndex = RandomInt(pixel, FrameIndex + sampleIndex * RESAMPLE_COUNT + proposalIndex, 4, gSceneConstantBuffer.PointLightInfosCount);
#else  // NOT SAMPLE_USING_FRAME_INDEX
        const uint pointLightIndex = RandomInt(pixel, sampleIndex * RESAMPLE_COUNT + proposalIndex, 4, gSceneConstantBuffer.PointLightInfosCount);
#endif // NOT SAMPLE_USING_FRAME_INDEX

        const float3 pointLightPosition = gPointLightInfos[pointLightIndex].Positions;
        const float3 hitPositionToPointLightPosition = pointLightPosition - hitPosition;
        const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
        const float distance = sqrt(distanceSquared);
        const float3 worldSampleDir = normalize(pointLightPosition - hitPosition);

        const float3 incomingRadiance = gPointLightInfos[pointLightIndex].Color * POINT_LIGHT_STRENGTH;
        const float geometryTerm = dot(worldSampleDir, normal) / distanceSquared;
        const float3 integrand = incomingRadiance * geometryTerm;
        const float desiredPdf = ConvertColorToLuminance(integrand);

        const float pdf = (1.0f / gSceneConstantBuffer.PointLightInfosCount);
        const float weight = desiredPdf / pdf;

        UpdatePointLightReservoir(outLightIndexSample, outDesiredPdf, outWeightsSum, outSamplesCount, weight, desiredPdf, pixel, sampleIndex, proposalIndex, bufferIndexByPixel);
    }

    return outWeightsSum / (outDesiredPdf * RESAMPLE_COUNT);
}

void
UpdateIndirectLightReservoir(out float3 outDirection, out float outDesiredPdf, inout float outWeightsSum, inout uint outSamplesCount, const float weight, const float desiredPdf, const float3 worldSampleDir, const uint2 pixel, const uint sampleIndex, const uint proposalIndex, const uint bufferIndexByPixel)
{
    outWeightsSum += weight;
    outSamplesCount += 1;
#if SAMPLE_USING_FRAME_INDEX
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + sampleIndex * RESAMPLE_COUNT + proposalIndex, 4);
#else  // NOT SAMPLE_USING_FRAME_INDEX
    const float canonicalSample = CanonicalSample(pixel, sampleIndex * RESAMPLE_COUNT + proposalIndex, 4);
#endif // NOT SAMPLE_USING_FRAME_INDEX
    const float probability = weight / outWeightsSum;
    if (canonicalSample < probability)
    {
        outDirection = worldSampleDir;
        outDesiredPdf = desiredPdf;
    }
}

float
RisIndirectLight(out float3 outDirection, out float outDesiredPdf, inout float outWeightsSum, inout uint outSamplesCount, const uint2 pixel, const uint sampleIndex, const float3 hitPosition, const float3 normal, const uint bufferIndexByPixel)
{
    for (uint proposalIndex = 0; proposalIndex < RESAMPLE_COUNT; ++proposalIndex)
    {
        // 1. Generate a sample: Cosine-weighted hemisphere sampling
        const float2 uniformSample = float2(
#if SAMPLE_USING_FRAME_INDEX
            CanonicalSample(pixel, FrameIndex + sampleIndex * RESAMPLE_COUNT + proposalIndex * 17, 0),
            CanonicalSample(pixel, FrameIndex + sampleIndex * RESAMPLE_COUNT + proposalIndex * 23, 1));
#else  // NOT SAMPLE_USING_FRAME_INDEX
            CanonicalSample(pixel, sampleIndex * RESAMPLE_COUNT + proposalIndex * 17, 0),
            CanonicalSample(pixel, sampleIndex * RESAMPLE_COUNT + proposalIndex * 23, 1));
#endif // NOT SAMPLE_USING_FRAME_INDEX
        const float3 localSampleDir = SampleHemisphereCosineWeighted(uniformSample);

        // Create an orthonormal basis (T, B, N) around the shading normal.
        float3 T;
        if (abs(normal.z) < 0.999f)
        {
            T = normalize(cross(normal, float3(0.0f, 0.0f, 1.0f)));
        }
        else
        {
            T = normalize(cross(normal, float3(0.0f, 1.0f, 0.0f)));
        }
        float3 B = cross(normal, T);

        // Transform the local sample direction to world space.
        const float3 worldSampleDir = localSampleDir.x * T + localSampleDir.y * B + localSampleDir.z * normal;

        const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
        const float desiredPdf = cosAtSurface;
        const float pdf = dot(worldSampleDir, normal) / PI;
        const float weight = desiredPdf / pdf;

        UpdateIndirectLightReservoir(outDirection, outDesiredPdf, outWeightsSum, outSamplesCount, weight, desiredPdf, worldSampleDir, pixel, sampleIndex, proposalIndex, bufferIndexByPixel);
    }

    return outWeightsSum / (outDesiredPdf * RESAMPLE_COUNT);
}

float4 
ComputeRenderingEquation(inout RayPayload payload, const float3 hitPosition, const float3 normal)
{
    const uint2 pixel = DispatchRaysIndex().xy;

    // Emissive radiance
    float3 emissiveRadiance = float3(0.0f, 0.0f, 0.0f);
    if (IsEmissive(PrimitiveIndex()) == 1)
    {
        const float tempEmissiveStrength = PARALLELOGRAM_AREA_LIGHT_STRENGTH;
        emissiveRadiance = gColors[PrimitiveIndex()] * tempEmissiveStrength;
    }

    float3 monteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    // NEE
    float3 neeMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    const uint bufferIndexByPixel = pixel.y * DispatchRaysDimensions().x + pixel.x;
    // Parallelogram area lights
    float3 parallelogramAreaLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
    if (payload.Depth > 0)
    {
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        float weightsSum = 0.0f;
        uint samplesCount = 0;
        for (uint sampleIndex = 0; sampleIndex < PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT; ++sampleIndex)
        {
            float3 samplePosition = float3(0.0f);
            float3 sampleNormal = float3(0.0f);
            float desiredPdf = 0.0f;

            const float weight = RisParallelogramAreaLight(samplePosition, sampleNormal, desiredPdf, weightsSum, samplesCount, pixel, sampleIndex, hitPosition, normal, bufferIndexByPixel);
            if(payload.Depth == BounceDepth)
            {
                gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightPositionSample = samplePosition;
                gParallelogramAreaLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
                gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightNormalSample = sampleNormal;
                gParallelogramAreaLightReservoir[bufferIndexByPixel].Weight = weight;
            }

            const float3 parallelogramAreaLightPosition = samplePosition;
            const float3 hitPositionToPointLightPosition = parallelogramAreaLightPosition - hitPosition;
            const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
            const float distance = sqrt(distanceSquared);
            const float3 worldSampleDir = normalize(parallelogramAreaLightPosition - hitPosition);

            // Trace a ray in the sampled direction.
            RayDesc ray;
            ray.Origin = hitPosition;
            ray.Direction = worldSampleDir;
            ray.TMin = 0.001;
            ray.TMax = 10000.0;

            RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, 0 };
            TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

            if (IsEmissive(newPayload.PrimitiveIndex) == 1)
            {
                const float3 incomingRadiance = newPayload.Color.rgb;
                const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
                const float cosAtLight = max(0.0f, dot(-worldSampleDir, sampleNormal));
                const float geometryTerm = cosAtSurface * cosAtLight / distanceSquared;

                if (cosAtLight > 0.0f)
                {
                    const float pdfInSolidAngle = (distanceSquared / cosAtLight) * desiredPdf;
                    const float hemisphereCosineWeightedPdf = cosAtSurface / PI;
                    const float misWeight = PowerHeuristic(PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, pdfInSolidAngle, POINT_LIGHT_SAMPLES_COUNT, 0.0f, HEMISPHERE_SAMPLES_COUNT, hemisphereCosineWeightedPdf);
                    // ReSTIR paper equation 2. Rendering equation.
                    //  L = ∫_A f(x) dx. where f(x) = ρ(x) * L_e(x) * G(x) * V(x)
                    //  ρ(x) = BRDF at x
                    //  L_e(x) = emissive radiance at x
                    //  G(x) = geometry term
                    //  V(x) = visibility term (1 if visible, 0 if not)
                    const float3 f = brdf * incomingRadiance * geometryTerm;
                    // ReSTIR paper equation 4. MIS.
                    //  <L>_mis^(M, N) = Σ_s=1^M (1 / N_s) Σ_i=1^N_s w_s(x_i) * (f(x_i) / p_s(x_i))
                    //  M = number of strategies (here: 3)
                    //  N_s = number of samples for strategy s
                    //  w_s(x_i) = MIS weight for strategy s and sample x_i
                    //  f(x_i) = integrand evaluated at sample x_i
                    //  p_s(x_i) = PDF for strategy s and sample x_i
                    // ReSTIR paper equation 7. multiple RIS.
                    //  <L>_ris^(N, M) = (1 / N) Σ_i=1^N ((f(y_i) / p_hat(y_i)) * ((1 / M) * Σ_j=1^M w(x_ij)))
                    // ReSTIR paper algorithm 3: Streaming RIS using weighted reservoir sampling.
                    //  f_q(r.y) * r.W

                    monteCarloIntegrandSum += f * misWeight * weight;
                }
            }
            else if (payload.Depth == BounceDepth)
            {
                gParallelogramAreaLightReservoir[bufferIndexByPixel].Weight = 0.0f;
            }
        }
        if (payload.Depth == BounceDepth)
        {
            gParallelogramAreaLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
            gParallelogramAreaLightReservoir[bufferIndexByPixel].SamplesCount = samplesCount;
        }
        parallelogramAreaLightMonteCarloEstimate = monteCarloIntegrandSum / PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT;
    }
#endif // PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
    neeMonteCarloEstimate += parallelogramAreaLightMonteCarloEstimate;

    // Point lights
    float3 pointLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if POINT_LIGHT_SAMPLES_COUNT > 0
    if (payload.Depth > 0)
    {
        // Point lights Integral
        // BRDF. Lambertian diffuse BRDF is constant, so we can take it out of the integral.
        // Note: For other BRDFs, the BRDF would need to be evaluated per sample.
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        float weight = 0.0f;
        float desiredPdf = 0.0f;
        float weightsSum = 0.0f;
        uint samplesCount = 0;
        for (uint sampleIndex = 0; sampleIndex < POINT_LIGHT_SAMPLES_COUNT; ++sampleIndex)
        {
            uint selectedProposalIndex = 0;
            if (RESAMPLE_COUNT <= gSceneConstantBuffer.PointLightInfosCount)
            {
                weight = RisPointLight(selectedProposalIndex, desiredPdf, weightsSum, samplesCount, pixel, sampleIndex, hitPosition, normal, bufferIndexByPixel);
                if (payload.Depth == BounceDepth)
                {
                    gPointLightReservoir[bufferIndexByPixel].Weight = weight;
                    gPointLightReservoir[bufferIndexByPixel].LightIndexSample = selectedProposalIndex;
                    gPointLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
                }
            }
            else
            {
                // Algorithm 2: Uniformly sample a point light
#if SAMPLE_USING_FRAME_INDEX
                selectedProposalIndex = RandomInt(pixel, FrameIndex + sampleIndex, 4, gSceneConstantBuffer.PointLightInfosCount);
#else  // NOT SAMPLE_USING_FRAME_INDEX
                selectedProposalIndex = RandomInt(pixel, sampleIndex, 4, gSceneConstantBuffer.PointLightInfosCount);
#endif // NOT SAMPLE_USING_FRAME_INDEX
                weight = gSceneConstantBuffer.PointLightInfosCount;
                desiredPdf = 1.0f / gSceneConstantBuffer.PointLightInfosCount;
            }

            const float3 pointLightPosition = gPointLightInfos[selectedProposalIndex].Positions;
            const float3 hitPositionToPointLightPosition = pointLightPosition - hitPosition;
            const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
            const float distance = sqrt(distanceSquared);
            const float3 worldSampleDir = normalize(pointLightPosition - hitPosition);

            // Trace a ray in the sampled direction.
            RayDesc ray;
            ray.Origin = hitPosition;
            ray.Direction = worldSampleDir;
            ray.TMin = 0.001;
            ray.TMax = 10000.0;

            RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, 0 };
            TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

            if (newPayload.RayT > distance)
            {
                const float3 incomingRadiance = gPointLightInfos[selectedProposalIndex].Color * POINT_LIGHT_STRENGTH;
                const float geometryTerm = dot(worldSampleDir, normal) / distanceSquared;

                // These are 0 because no parallelogram area light sampling nor hemisphere sampling can sample point lights
                float parallelogramAreaLightPdf = 0.0f;
                float hemisphereSamplePdf = 0.0f;
                const float misWeight = PowerHeuristic(POINT_LIGHT_SAMPLES_COUNT, desiredPdf, HEMISPHERE_SAMPLES_COUNT, hemisphereSamplePdf, PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, parallelogramAreaLightPdf);

                const float3 f = brdf * incomingRadiance * geometryTerm;

                monteCarloIntegrandSum += f * misWeight * weight;
            }
            else if (payload.Depth == BounceDepth)
            {
                gPointLightReservoir[bufferIndexByPixel].Weight = 0.0f;
            }
        }
        if (payload.Depth == BounceDepth)
        {
            gPointLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
            gPointLightReservoir[bufferIndexByPixel].SamplesCount = samplesCount;
        }
        pointLightMonteCarloEstimate = monteCarloIntegrandSum / POINT_LIGHT_SAMPLES_COUNT;
    }
#endif // POINT_LIGHT_SAMPLES_COUNT > 0
    neeMonteCarloEstimate += pointLightMonteCarloEstimate;

    // Indirect Illumination Integral
    float3 indirectIlluminationMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if HEMISPHERE_SAMPLES_COUNT > 0
    if (payload.Depth > 0)
    {
        // BRDF. Lambertian diffuse BRDF is constant, so we can take it out of the integral.
        // Note: For other BRDFs, the BRDF would need to be evaluated per sample.
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        float weightsSum = 0.0f;
        uint samplesCount = 0;
        if(payload.Depth == BounceDepth)
        {
            weightsSum = gIndirectLightReservoir[bufferIndexByPixel].WeightSum;
            samplesCount = gIndirectLightReservoir[bufferIndexByPixel].SamplesCount;
        }
        for (uint sampleIndex = 0; sampleIndex < HEMISPHERE_SAMPLES_COUNT; ++sampleIndex)
        {
            float3 worldSampleDir = float3(0.0f);
            float desiredPdf = 0.0f;
            const float weight = RisIndirectLight(worldSampleDir, desiredPdf, weightsSum, samplesCount, pixel, sampleIndex, hitPosition, normal, bufferIndexByPixel);
            if(payload.Depth == BounceDepth)
            {
                gIndirectLightReservoir[bufferIndexByPixel].Direction = worldSampleDir;
                gIndirectLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
                gIndirectLightReservoir[bufferIndexByPixel].Weight = weight;
            }

            // Trace a ray in the sampled direction.
            RayDesc ray;
            ray.Origin = hitPosition;
            ray.Direction = worldSampleDir;
            ray.TMin = 0.001;
            ray.TMax = 10000.0;
            
            RayPayload newPayload = { float4(0, 0, 0, 0), 0.0f, 0, payload.Depth - 1 };
            TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, newPayload);

            // 3. Incoming radiance
            const float3 incomingRadiance = newPayload.Color.rgb;
            
            // 4. Cosine term is embedded in the sampling pdf for cosine-weighted hemisphere sampling.

            // 5. Integrand
            const float pointLightPdf = 0.0f;                   // assume that the sampled direction doesn't hit a point light
            float parallelogramAreaLightPdf = 0.0f;
            if (newPayload.PrimitiveIndex != 0xFFFFFFFF && IsEmissive(newPayload.PrimitiveIndex) != 1)
            {
                // const ParallelogramAreaLightInfo lightInfo = gParallelogramAreaLightInfos[0];
                // const float3 e1 = lightInfo.Positions[1] - lightInfo.Positions[0];
                // const float3 e2 = lightInfo.Positions[2] - lightInfo.Positions[0];
                // const float parallelogramArea = length(cross(e1, e2));
                // parallelogramAreaLightPdf = 1.0f / parallelogramArea;
                const float misWeight = PowerHeuristic(HEMISPHERE_SAMPLES_COUNT, desiredPdf, POINT_LIGHT_SAMPLES_COUNT, pointLightPdf, PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, parallelogramAreaLightPdf);

                const float3 f = brdf * incomingRadiance;
                monteCarloIntegrandSum += f * misWeight * weight;
            }
            else if (payload.Depth == BounceDepth)
            {
                gIndirectLightReservoir[bufferIndexByPixel].Weight = 0.0f;
            }
        }

        if (payload.Depth == BounceDepth)
        {
            gIndirectLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
            gIndirectLightReservoir[bufferIndexByPixel].SamplesCount = samplesCount;
        }
        indirectIlluminationMonteCarloEstimate = monteCarloIntegrandSum / HEMISPHERE_SAMPLES_COUNT;
    }
#endif // HEMISPHERE_SAMPLES_COUNT > 0

    monteCarloEstimate += neeMonteCarloEstimate;
    monteCarloEstimate += indirectIlluminationMonteCarloEstimate;

    // 5) write
    return float4(emissiveRadiance + monteCarloEstimate, 1.0f);
}

[shader("raygeneration")]
void RayGenMain()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    const uint bufferIndexByPixel = pixel.y * DispatchRaysDimensions().x + pixel.x;

    // ReSTIR paper algorithm 3: Streaming RIS using weighted reservoir sampling.
    //  Reservoir r
    InitializeParallelogramAreaLightReservoir(gParallelogramAreaLightReservoir[bufferIndexByPixel]);
    InitializePointLightReservoir(gPointLightReservoir[bufferIndexByPixel]);
    InitializeIndirectLightReservoir(gIndirectLightReservoir[bufferIndexByPixel]);

    float3 rayDir;
    float3 origin;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(pixel, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0), 0.0f, 0, BounceDepth, bufferIndexByPixel };
    uint seed = RandomSeed;
    EncryptTea(payload.Seed, seed);
    TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    // float3 sceneLinearColor = payload.Color.rgb;

    // 1) fireflies/NaNs guard (optional but wise for path tracing)
    // sceneLinearColor = max(sceneLinearColor, float3(0.0, 0.0, 0.0)); // avoid NaN and negative values
    // sceneLinearColor = min(sceneLinearColor, float3(1e6, 1e6, 1e6)); // avoid INF

    // 2) exposure (tweak per scene/camera)
    // sceneLinearColor = ApplyExposure(sceneLinearColor, /*exposureEV*/ 0.0);

    // 3) tonemap (choose one)
    // const float3 tonemapped = TonemapACES(sceneLinearColor);

    // 4) sRGB encode for UNORM output
    // const float3 srgb = LinearToSRGB(tonemapped);

    // 5) write
    // gRenderTarget[DispatchRaysIndex().xy] = float4(saturate(srgb), payload.Color.a);
}


[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    const float3 hitPosition = HitWorldPosition();

    const float3 triangleNormal = GetWorldNormal(PrimitiveIndex(), attr.barycentrics);

    if(payload.Depth == BounceDepth)
    {
        gGBuffers[DispatchRaysIndex().xy] = float4(attr.barycentrics, PrimitiveIndex(), 1.0f);
    }

    const float4 color = ComputeRenderingEquation(payload, hitPosition, triangleNormal);

    payload.Color = color;
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
    if(payload.Depth == BounceDepth)
    {
        gGBuffers[DispatchRaysIndex().xy] = float4(0.0f, 0.0f, -1.0f, 1.0f);
    }
}