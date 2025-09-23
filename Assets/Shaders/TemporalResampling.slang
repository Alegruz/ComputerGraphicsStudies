// shaders.slang

#define FLT_MAX         (3.402823466e+38)
#define PI              (3.1415926535897f)

struct SceneConstantBuffer
{
    float4x4 ProjectionToWorldTransformMatrix;
    float4 CameraPosition;
    uint ParallelogramAreaLightInfosCount;
    uint PointLightInfosCount;
};

struct VertexPN
{
    float3 Position;
    float3 Normal;
};

struct ParallelogramAreaLightInfo
{
    float3 Positions[3];
    float3 Color;
    uint PrimitiveIndices[2];
};

struct PointLightInfo
{
    float3 Positions;
    float3 Color;
    float Radius;
};

struct ParallelogramAreaLightReservoir
{
    float3 ParallelogramAreaLightPositionSample;
    float Pdf;
    float3 ParallelogramAreaLightNormalSample;
    float WeightSum;

    float3 Weight;
    uint SamplesCount;
};

struct PointLightReservoir
{
    float3 Weight;
    uint LightIndexSample;

    float WeightSum;
    float Pdf;

    uint SamplesCount;
};

struct IndirectLightReservoir
{
    float3 Direction;
    float Pdf;

    float3 Weight;
    float WeightSum;

    uint SamplesCount;
};

uniform RaytracingAccelerationStructure gScene;
uniform RWTexture2D gRenderTarget;
uniform RWTexture2D<float4> gGBuffers;
uniform RWStructuredBuffer<ParallelogramAreaLightReservoir> gParallelogramAreaLightReservoir;
uniform RWStructuredBuffer<PointLightReservoir> gPointLightReservoir;
uniform RWStructuredBuffer<IndirectLightReservoir> gIndirectLightReservoir;
uniform RWStructuredBuffer<ParallelogramAreaLightReservoir> gPrevParallelogramAreaLightReservoir;
uniform RWStructuredBuffer<PointLightReservoir> gPrevPointLightReservoir;
uniform RWStructuredBuffer<IndirectLightReservoir> gPrevIndirectLightReservoir;
uniform ByteAddressBuffer gIndices;
uniform StructuredBuffer<VertexPN> gVertices;
uniform StructuredBuffer<float3> gColors;
uniform ByteAddressBuffer gIsEmissive;
uniform StructuredBuffer<ParallelogramAreaLightInfo> gParallelogramAreaLightInfos;
uniform StructuredBuffer<PointLightInfo> gPointLightInfos;

uniform ConstantBuffer<SceneConstantBuffer> gSceneConstantBuffer;

cbuffer PushConstants
{
    uint FrameIndex;
    uint BounceDepth;
};

float
ConvertColorToLuminance(const float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// --- exposure (key) ---
float3
ApplyExposure(const float3 radiance, const float exposure)
{
    // exposure in stops; positive brightens, negative darkens
    return radiance * exp2(exposure);
}

// --- simple tonemap (ACES-approx or Reinhard) ---
float3
TonemapACES(const float3 x)
{
    // Narkowicz ACES approximation (good + cheap)
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3
TonemapReinhard(const float3 x)
{
    return x / (1.0 + x);
}

// --- sRGB encode (do this because UAV is UNORM, not SRGB) ---
float3
LinearToSRGB(const float3 x)
{
    float3 lo = 12.92 * x;
    float3 hi = 1.055 * pow(max(x, float3(0.0, 0.0, 0.0)), 1.0 / 2.4) - 0.055;
    return lerp(lo, hi, step(0.0031308, x));
}

// Load three 16 bit indices from a byte addressed buffer.
uint3 
Load3x16BitIndices(uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;    
    const uint2 four16BitIndices = gIndices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

uint
IsEmissive(const uint index)
{
    const uint isEmissive32Data = gIsEmissive.Load(index);
    const uint isEmissive = (isEmissive32Data & (0xFF << (index * 8))) >> (index * 8);
    return isEmissive;
}

#if 1
struct RayPayload
{
    float4 Color;
    float RayT;
    uint PrimitiveIndex;
    uint Depth;
};
#else
struct [raypayload] RayPayload
{
    float4 Color : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    float RayT : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    uint PrimitiveIndex : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    uint Depth : read(closesthit, miss, caller) : write(closesthit, miss, caller);
};
#endif

// Retrieve hit world position.
float3 
HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float3 
InterpolateByBarycentric(float3 vertexAttribute[3], const float2 barycentrics)
{
    return vertexAttribute[0] +
        barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3
HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return InterpolateByBarycentric(vertexAttribute, attr.barycentrics);
}

float3
GetWorldNormal(const uint primitiveIndex, const float2 barycentrics)
{
    // Get the base index of the triangle's first 16 bit index.
    const uint indexSizeInBytes = 2;
    const uint indicesPerTriangle = 3;
    const uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    const uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] =
        {
            gVertices[indices[0]].Normal,
            gVertices[indices[1]].Normal,
            gVertices[indices[2]].Normal
        };

    // Compute the triangle's normal.
    // This is redundant and done for illustration purposes
    // as all the per-vertex normals are the same and match triangle's normal in this sample.
    const float3 triangleNormal = InterpolateByBarycentric(vertexNormals, barycentrics);
    return triangleNormal;
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void 
GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(gSceneConstantBuffer.ProjectionToWorldTransformMatrix, float4(screenPos, 0, 1));

    world.xyz /= world.w;
    origin = gSceneConstantBuffer.CameraPosition.xyz;
    direction = normalize(world.xyz - origin);
}

float 
HashToUnitFloat(uint seed)
{
    seed ^= seed >> 17;
    seed *= 0xed5ad4bb;
    seed ^= seed >> 11;
    seed *= 0xac4c1b51;
    seed ^= seed >> 15;
    seed *= 0x31848bab;
    seed ^= seed >> 14;
    return (seed & 0x00FFFFFF) / 16777216.0; // [0,1)
}

float3 
SampleParallelogram(const float3 p0, const float3 e1, const float3 e2, const float2 u)
{
    return p0 + u.x * e1 + u.y * e2;
}

float 
CanonicalSample(const uint2 pixel, const uint frameIndex, const uint dimension)
{
    uint seed = pixel.x * 73856093u   // big prime
              ^ pixel.y * 19349663u   // another big prime
              ^ frameIndex * 83492791u
              ^ dimension * 2654435761u; // 2^32 / golden ratio
    return HashToUnitFloat(seed);
}

float3
SampleHemisphereUniform(const float2 uniformSample)
{
    const float z = uniformSample.x;
    const float r = sqrt(max(0.0f, 1.0f - z * z));
    const float phi = 2.0f * PI * uniformSample.y;
    const float x = r * cos(phi);
    const float y = r * sin(phi);
    return float3(x, y, z);
}

float3
SampleHemisphereCosineWeighted(const float2 uniformSample)
{
    const float r = sqrt(uniformSample.x);
    const float theta = 2.0f * PI * uniformSample.y;
    const float x = r * cos(theta);
    const float y = r * sin(theta);
    const float z = sqrt(1.0f - uniformSample.x);
    return float3(x, y, z);
}

uint 
RandomInt(const uint2 pixel, const uint frameIndex, const uint dimension, const uint maxN)
{
    // Mix in pixel, frame, and dimension for decorrelation
    const uint seed = pixel.x * 73856093u
              ^ pixel.y * 19349663u
              ^ frameIndex * 83492791u
              ^ dimension * 2654435761u;

    const float rnd = HashToUnitFloat(seed);
    return (uint)(rnd * maxN); // uniformly in [0, maxN-1]
}

float3
CalculateLambertianDiffuseBrdf(const uint primitiveIndex)
{
    // Lambertian diffuse BRDF
    const float3 baseColor = gColors[primitiveIndex];
    return baseColor / PI;
}

float
PowerHeuristic(const uint samplesCountA, const float pdfA, const uint samplesCountB, const float pdfB, const uint samplesCountC, const float pdfC)
{
    const float a = samplesCountA * pdfA;
    const float b = samplesCountB * pdfB;
    const float c = samplesCountC * pdfC;
    return (a * a) / (a * a + b * b + c * c);
}

#define PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT (1)
#define POINT_LIGHT_SAMPLES_COUNT (1)
#define HEMISPHERE_SAMPLES_COUNT (1)
#define RESAMPLE_COUNT (16)

void
UpdateParallelogramAreaLightReservoir(const float weight, const float desiredPdf, const float3 parallelogramAreaLightPosition, const float3 triangleNormal, const uint2 pixel, const uint bufferIndexByPixel, const uint samplesCount)
{
    float weightsSum = gParallelogramAreaLightReservoir[bufferIndexByPixel].WeightSum + weight;
    gParallelogramAreaLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
    gParallelogramAreaLightReservoir[bufferIndexByPixel].SamplesCount = gParallelogramAreaLightReservoir[bufferIndexByPixel].SamplesCount + samplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + samplesCount * 11, 4);
    const float probability = weight / weightsSum;
    if (canonicalSample < probability)
    {
        gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightPositionSample = parallelogramAreaLightPosition;
        gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightNormalSample = triangleNormal;
        gParallelogramAreaLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
    }
}
void
UpdatePointLightReservoir(const float weight, const float desiredPdf, const uint2 pixel, const uint proposalIndex, const uint bufferIndexByPixel, const uint samplesCount)
{
    float weightsSum = gPointLightReservoir[bufferIndexByPixel].WeightSum + weight;
    gPointLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
    gPointLightReservoir[bufferIndexByPixel].SamplesCount = gPointLightReservoir[bufferIndexByPixel].SamplesCount + samplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + samplesCount * proposalIndex * 23, 4);
    const float probability = weight / weightsSum;
    if (canonicalSample < probability)
    {
        gPointLightReservoir[bufferIndexByPixel].LightIndexSample = proposalIndex;
        gPointLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
    }
}

void
UpdateIndirectLightReservoir(const float weight, const float desiredPdf, const float3 worldSampleDir, const uint2 pixel, const uint bufferIndexByPixel, const uint samplesCount)
{
    const float weightsSum = gIndirectLightReservoir[bufferIndexByPixel].WeightSum + weight;
    gIndirectLightReservoir[bufferIndexByPixel].WeightSum = weightsSum;
    gIndirectLightReservoir[bufferIndexByPixel].SamplesCount = gIndirectLightReservoir[bufferIndexByPixel].SamplesCount + samplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + samplesCount * 17, 4);
    const float probability = weight / weightsSum;
    if (canonicalSample < probability)
    {
        gIndirectLightReservoir[bufferIndexByPixel].Direction = worldSampleDir;
        gIndirectLightReservoir[bufferIndexByPixel].Pdf = desiredPdf;
    }
}
[shader("raygeneration")]
void RayGenMain()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    const uint bufferIndexByPixel = pixel.y * DispatchRaysDimensions().x + pixel.x;

    const float4 gBufferData = gGBuffers[pixel];
    const float2 barycentrics = gBufferData.xy;
    const uint primitiveIndex = (uint)gBufferData.z;

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

    float3 monteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    float3 neeMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    const float3 brdf = CalculateLambertianDiffuseBrdf(primitiveIndex);
    float3 parallelogramAreaLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
    {
        {
            const float desiredPdf = gPrevParallelogramAreaLightReservoir[bufferIndexByPixel].Pdf;
            const float weight = ConvertColorToLuminance(gPrevParallelogramAreaLightReservoir[bufferIndexByPixel].Weight * desiredPdf);
            const float3 position = gPrevParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightPositionSample;
            const float3 normal = gPrevParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightNormalSample;
            const uint samplesCount = gPrevParallelogramAreaLightReservoir[bufferIndexByPixel].SamplesCount;
            UpdateParallelogramAreaLightReservoir(weight, weight, position, normal, pixel, bufferIndexByPixel, samplesCount);
        }

        const float3 selectedProposalPosition = gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightPositionSample;
        const float3 selectedProposalNormal = gParallelogramAreaLightReservoir[bufferIndexByPixel].ParallelogramAreaLightNormalSample;
        const float pdf = gParallelogramAreaLightReservoir[bufferIndexByPixel].Pdf;
        const float3 weight = gParallelogramAreaLightReservoir[bufferIndexByPixel].Weight;

        const float3 parallelogramAreaLightPosition = selectedProposalPosition;
        const float3 hitPositionToPointLightPosition = parallelogramAreaLightPosition - position;
        const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
        const float distance = sqrt(distanceSquared);
        const float3 worldSampleDir = normalize(parallelogramAreaLightPosition - position);

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
    neeMonteCarloEstimate += parallelogramAreaLightMonteCarloEstimate;

    float3 pointLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
    {
        {
            const float desiredPdf = gPrevPointLightReservoir[bufferIndexByPixel].Pdf;
            const float weight = ConvertColorToLuminance(gPrevPointLightReservoir[bufferIndexByPixel].Weight * desiredPdf);
            const uint proposalIndex = gPrevPointLightReservoir[bufferIndexByPixel].LightIndexSample;
            const uint samplesCount = gPrevPointLightReservoir[bufferIndexByPixel].SamplesCount;
            UpdatePointLightReservoir(weight, weight, pixel, proposalIndex, bufferIndexByPixel, samplesCount);
        }

        const uint selectedProposalIndex = gPointLightReservoir[bufferIndexByPixel].LightIndexSample;
        const float pdf = gPointLightReservoir[bufferIndexByPixel].Pdf;
        const float3 weight = gPointLightReservoir[bufferIndexByPixel].Weight;
        const float3 pointLightPosition = gPointLightInfos[selectedProposalIndex].Positions;
        const float3 hitPositionToPointLightPosition = pointLightPosition - position;
        const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
        const float distance = sqrt(distanceSquared);
        const float3 worldSampleDir = normalize(pointLightPosition - position);

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
    neeMonteCarloEstimate += pointLightMonteCarloEstimate;

    float3 indirectIlluminationMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
    {
        {
            const float desiredPdf = gPrevIndirectLightReservoir[bufferIndexByPixel].Pdf;
            const float weight = ConvertColorToLuminance(gPrevIndirectLightReservoir[bufferIndexByPixel].Weight * desiredPdf);
            const float3 direction = gPrevIndirectLightReservoir[bufferIndexByPixel].Direction;
            const uint samplesCount = gPrevIndirectLightReservoir[bufferIndexByPixel].SamplesCount;
            UpdateIndirectLightReservoir(weight, weight, direction, pixel, bufferIndexByPixel, samplesCount);
        }

        const float3 weight = gIndirectLightReservoir[bufferIndexByPixel].Weight;
        const float3 worldSampleDir = gIndirectLightReservoir[bufferIndexByPixel].Direction;
        const float pdf = gIndirectLightReservoir[bufferIndexByPixel].Pdf;

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
        const float3 integrand = incomingRadiance * misWeight * weight;
        indirectIlluminationMonteCarloEstimate = integrand * brdf;
    }

    monteCarloEstimate += neeMonteCarloEstimate;
    monteCarloEstimate += indirectIlluminationMonteCarloEstimate;

    // Write the raytraced color to the output texture.
    float3 sceneLinearColor = monteCarloEstimate.rgb;

    // 1) fireflies/NaNs guard (optional but wise for path tracing)
    sceneLinearColor = max(sceneLinearColor, float3(0.0, 0.0, 0.0)); // avoid NaN and negative values
    sceneLinearColor = min(sceneLinearColor, float3(1e6, 1e6, 1e6)); // avoid INF

    // 2) exposure (tweak per scene/camera)
    sceneLinearColor = ApplyExposure(sceneLinearColor, /*exposureEV*/ 0.0);

    // 3) tonemap (choose one)
    const float3 tonemapped = TonemapACES(sceneLinearColor);

    // 4) sRGB encode for UNORM output
    const float3 srgb = LinearToSRGB(tonemapped);

    // 5) write
    gRenderTarget[DispatchRaysIndex().xy] = float4(saturate(srgb), 1.0f);
}


[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.Color = float4(gColors[PrimitiveIndex()], 1.0f);
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