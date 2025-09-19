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

uniform RaytracingAccelerationStructure gScene;
uniform RWTexture2D gRenderTarget;
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
#define HEMISPHERE_SAMPLES_COUNT (2)

float4 
ComputeRenderingEquation(inout RayPayload payload, const float3 hitPosition, const float3 normal)
{
    const uint2 pixel = DispatchRaysIndex().xy;

    // Emissive radiance
    float3 emissiveRadiance = float3(0.0f, 0.0f, 0.0f);
    if (IsEmissive(PrimitiveIndex()) == 1)
    {
        const float tempEmissiveStrength = 40.0f;
        emissiveRadiance = gColors[PrimitiveIndex()] * tempEmissiveStrength;
    }

    float3 monteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    // NEE
    float3 neeMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);

    // Parallelogram area lights
    float3 parallelogramAreaLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
    if (payload.Depth > 0)
    {
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        for (uint sampleIndex = 0; sampleIndex < PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT; ++sampleIndex)
        {
            const uint parallelogramLightIndex = 0;
            const ParallelogramAreaLightInfo lightInfo = gParallelogramAreaLightInfos[parallelogramLightIndex];
            const float2 uniformSample = float2(
                CanonicalSample(pixel, FrameIndex + sampleIndex * 2, 0),
                CanonicalSample(pixel, FrameIndex + sampleIndex * 2 + 1, 1));
            const float3 p0 = lightInfo.Positions[0];
            const float3 e1 = lightInfo.Positions[1] - lightInfo.Positions[0];
            const float3 e2 = lightInfo.Positions[2] - lightInfo.Positions[0];

            const float parallelogramArea = length(cross(e1, e2));

            const float3 pointLightPosition = SampleParallelogram(p0, e1, e2, uniformSample);
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

            if (IsEmissive(newPayload.PrimitiveIndex) == 1)
            {
                // Calculate barycentrics for the hit point on the triangle using primitive index, RayT, and worldSampleDir
                float3 hitPoint = ray.Origin + newPayload.RayT * ray.Direction;

                const uint indexSizeInBytes = 2;
                const uint indicesPerTriangle = 3;
                const uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
                const uint baseIndex = newPayload.PrimitiveIndex * triangleIndexStride;
                const uint3 indices = Load3x16BitIndices(baseIndex);

                float3 v0 = gVertices[indices[0]].Position;
                float3 v1 = gVertices[indices[1]].Position;
                float3 v2 = gVertices[indices[2]].Position;

                // Compute barycentric coordinates of hitPoint with respect to triangle (v0, v1, v2)
                float3 v0v1 = v1 - v0;
                float3 v0v2 = v2 - v0;
                float3 v0p = hitPoint - v0;
                float d00 = dot(v0v1, v0v1);
                float d01 = dot(v0v1, v0v2);
                float d11 = dot(v0v2, v0v2);
                float d20 = dot(v0p, v0v1);
                float d21 = dot(v0p, v0v2);
                float denom = d00 * d11 - d01 * d01;
                float v = (d11 * d20 - d01 * d21) / denom;
                float w = (d00 * d21 - d01 * d20) / denom;
                float u = 1.0f - v - w;
                float2 barycentrics = float2(v, w);

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

                const float3 incomingRadiance = newPayload.Color.rgb;
                const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
                const float cosAtLight = max(0.0f, dot(-worldSampleDir, triangleNormal));
                const float geometryTerm = cosAtSurface * cosAtLight / distanceSquared;

                const float pdfInArea = 1.0f / parallelogramArea;
                if (cosAtLight > 0.0f)
                {
                    const float pdfInSolidAngle = (distanceSquared / cosAtLight) * pdfInArea;
                    const float hemisphereCosineWeightedPdf = cosAtSurface / PI;
                    const float misWeight = PowerHeuristic(PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, pdfInSolidAngle, POINT_LIGHT_SAMPLES_COUNT, 0.0f, HEMISPHERE_SAMPLES_COUNT, hemisphereCosineWeightedPdf);
                    const float3 integrand = incomingRadiance * geometryTerm * misWeight;

                    monteCarloIntegrandSum += (integrand / pdfInArea);
                }
            }
        }
        parallelogramAreaLightMonteCarloEstimate = (monteCarloIntegrandSum * brdf) / PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT;
    }
#endif // PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT > 0
    neeMonteCarloEstimate += parallelogramAreaLightMonteCarloEstimate;

    // Point lights
    float weights[64] = { 0.0f, };
    float3 pointLightMonteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
#if POINT_LIGHT_SAMPLES_COUNT > 0
    if (payload.Depth > 0)
    {
        // Point lights Integral
        // BRDF. Lambertian diffuse BRDF is constant, so we can take it out of the integral.
        // Note: For other BRDFs, the BRDF would need to be evaluated per sample.
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        float totalDistances = 0.0f;
        for (uint lightIndex = 0; lightIndex < gSceneConstantBuffer.PointLightInfosCount; ++lightIndex)
        {
            const float3 hitPositionToPointLight = hitPosition - gPointLightInfos[lightIndex].Positions;
            const float distanceSquared = dot(hitPositionToPointLight, hitPositionToPointLight);
            const float distance = sqrt(distanceSquared);
            const float minDistance = max(0.001f, distance - gPointLightInfos[lightIndex].Radius);
            weights[lightIndex] = minDistance;
            totalDistances += weights[lightIndex];
        }

        for (uint lightIndex = 0; lightIndex < gSceneConstantBuffer.PointLightInfosCount; ++lightIndex)
        {
            weights[lightIndex] /= totalDistances;
        }

        for (uint sampleIndex = 0; sampleIndex < POINT_LIGHT_SAMPLES_COUNT; ++sampleIndex)
        {
            const float canonicalSample = CanonicalSample(pixel, FrameIndex + sampleIndex, 3);
            uint pointLightIndex = 0;
            float cumulativeWeight = 0.0f;
            for (uint lightIndex = 0; lightIndex < gSceneConstantBuffer.PointLightInfosCount; ++lightIndex)
            {
                cumulativeWeight += weights[lightIndex];
                if (canonicalSample < cumulativeWeight)
                {
                    pointLightIndex = lightIndex;
                    break;
                }
            }

            const float3 pointLightPosition = gPointLightInfos[pointLightIndex].Positions;
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
                const float pointLightStrength = 100.0f;
                const float3 incomingRadiance = gPointLightInfos[pointLightIndex].Color * pointLightStrength;
                const float geometryTerm = dot(worldSampleDir, normal) / distanceSquared;

                const float pdf = dot(worldSampleDir, normal) / PI;
                float parallelogramAreaLightPdf = 0.0f;
                if (newPayload.PrimitiveIndex != 0xFFFFFFFF && IsEmissive(newPayload.PrimitiveIndex) == 1)
                {
                    const ParallelogramAreaLightInfo lightInfo = gParallelogramAreaLightInfos[0];
                    const float3 e1 = lightInfo.Positions[1] - lightInfo.Positions[0];
                    const float3 e2 = lightInfo.Positions[2] - lightInfo.Positions[0];
                    const float parallelogramArea = length(cross(e1, e2));
                    parallelogramAreaLightPdf = 1.0f / parallelogramArea;
                }
                const float misWeight = PowerHeuristic(POINT_LIGHT_SAMPLES_COUNT, weights[pointLightIndex], HEMISPHERE_SAMPLES_COUNT, pdf, PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT, parallelogramAreaLightPdf);
                const float3 integrand = incomingRadiance * geometryTerm * misWeight;

                monteCarloIntegrandSum += (integrand / weights[pointLightIndex]);
            }
        }
        pointLightMonteCarloEstimate = (monteCarloIntegrandSum * brdf) / POINT_LIGHT_SAMPLES_COUNT;
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
        for (uint sampleIndex = 0; sampleIndex < HEMISPHERE_SAMPLES_COUNT; ++sampleIndex)
        {
            // 1. Generate a sample: Cosine-weighted hemisphere sampling
            const float2 uniformSample = float2(
                CanonicalSample(pixel, FrameIndex + sampleIndex * 2, 0),
                CanonicalSample(pixel, FrameIndex + sampleIndex * 2 + 1, 1));
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
            const float pdf = dot(worldSampleDir, normal) / PI; // cosine-weighted hemisphere pdf
            const float pointLightPdf = 0.0f;                   // assume that the sampled direction doesn't hit a point light
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
            const float3 integrand = incomingRadiance * misWeight;
            monteCarloIntegrandSum += integrand;
        }
        indirectIlluminationMonteCarloEstimate = (monteCarloIntegrandSum * brdf) / HEMISPHERE_SAMPLES_COUNT;
    }
#endif // HEMISPHERE_SAMPLES_COUNT > 0

    monteCarloEstimate += neeMonteCarloEstimate;
    monteCarloEstimate += indirectIlluminationMonteCarloEstimate;

    return float4(emissiveRadiance + monteCarloEstimate, 1.0f);
}

[shader("raygeneration")]
void RayGenMain()
{
    float3 rayDir;
    float3 origin;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0), 0.0f, 0, BounceDepth };
    TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    gRenderTarget[DispatchRaysIndex().xy] = payload.Color;
}


[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    const float3 hitPosition = HitWorldPosition();

    const float3 triangleNormal = GetWorldNormal(PrimitiveIndex(), attr.barycentrics);

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
}