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

struct [raypayload] RayPayload
{
    float4 Color : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    float RayT : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    uint PrimitiveIndex : read(closesthit, miss, caller) : write(closesthit, miss, caller);
    uint Depth : read(closesthit, miss, caller) : write(closesthit, miss, caller);
};

// Retrieve hit world position.
float3 
HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 
HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
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

float4
CalculateLambertianDiffuseBrdf()
{
    // Lambertian diffuse BRDF
    return float4(1.0f / PI, 1.0f / PI, 1.0f / PI, 1.0f);
}

float4 
EvaluateRadiance(const float3 lightToHitPosition, const float3 lightColor, const float radius, const float3 normal)
{
    const float3 lightDirection = normalize(lightToHitPosition);

    // Mutual visibility

    // BSDF
    const float4 brdf = CalculateLambertianDiffuseBrdf();

    // Emitted radiance
    
    // Geometry term
    // inverse squared distance
    float distanceAttenuation = 1.0f;
    if(radius > 0.0f)
    {
        const float distanceSqr = dot(lightToHitPosition, lightToHitPosition);
        const float eta = 0.01f;
        distanceAttenuation = radius * radius / (distanceSqr + eta);
    }
    // cosine foreshortening term at light source
    float fNDotL = max(0.0f, dot(lightDirection, normal));

    return float4(lightColor * distanceAttenuation * fNDotL, 1.0f) * brdf;
}

float4 
ComputeRenderingEquation(inout RayPayload payload, const float3 hitPosition, const float3 normal)
{
    const uint2 pixel = DispatchRaysIndex().xy;

    const uint parallelogramLightRaysCount = 3;
    float4 parallelogramLightContribution = float4(0, 0, 0, 0);
    {
        if(payload.Depth > 0)
        {
            const uint randomParallelogramAreaLightInfoIndex = RandomInt(pixel, FrameIndex, PrimitiveIndex() + 0, gSceneConstantBuffer.ParallelogramAreaLightInfosCount);

            // Given emissive geometry corners p0, p1, p3 (parallelogram assumption)
            const float3 p0 = gParallelogramAreaLightInfos[randomParallelogramAreaLightInfoIndex].Positions[0];
            const float3 p1 = gParallelogramAreaLightInfos[randomParallelogramAreaLightInfoIndex].Positions[1];
            const float3 p3 = gParallelogramAreaLightInfos[randomParallelogramAreaLightInfoIndex].Positions[2];

            const float3 e1 = p1 - p0;
            const float3 e2 = p3 - p0;

            // Compute the area of the parallelogram
            const float3 parallelogramNormal = cross(e1, e2);
            const float3 parallelogramNormalizedNormal = normalize(parallelogramNormal);
            const float parallelogramArea = length(parallelogramNormal);
            const float parallelogramAreaPdf = 1.0f / parallelogramArea;

            float3 lightColor = gParallelogramAreaLightInfos[randomParallelogramAreaLightInfoIndex].Color;
            const float radius = 0.0f;

            for (uint rayIndex = 0; rayIndex < parallelogramLightRaysCount; ++rayIndex)
            {
                // Generate two randoms
                float2 u;
                u.x = CanonicalSample(pixel, FrameIndex, (PrimitiveIndex() + 0) + rayIndex);
                u.y = CanonicalSample(pixel, FrameIndex, (PrimitiveIndex() + 1) + rayIndex);

                // Sampled point
                float3 lightPosition = SampleParallelogram(p0, e1, e2, u);
                float3 lightToHitPosition = lightPosition - hitPosition;
                const float3 lightDirection = normalize(lightToHitPosition);

                RayDesc ray;
                ray.Origin = hitPosition;
                ray.Direction = lightDirection;
                // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
                // TMin should be kept small to prevent missing geometry at close contact areas.
                ray.TMin = 0.001;
                ray.TMax = 10000.0;
                RayPayload shadowRayPayload;
                shadowRayPayload.Color = float4(0, 0, 0, 0);
                shadowRayPayload.RayT = 0.0f;
                shadowRayPayload.PrimitiveIndex = 0xFFFFFFFF;
                shadowRayPayload.Depth = 0; // Only check for visibility
                TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, shadowRayPayload);
                if (shadowRayPayload.PrimitiveIndex != 0xFFFFFFFF)
                {
                    const uint isEmissive = IsEmissive(shadowRayPayload.PrimitiveIndex);
                    if (isEmissive == 1)
                    {
                        const float3 areaRadiance = lightColor * (100.0f / (parallelogramArea * PI));
                        float4 singleParallelogramLightContribution = EvaluateRadiance(lightToHitPosition, areaRadiance, radius, normal);
                        const float pdf = parallelogramAreaPdf * dot(lightToHitPosition, lightToHitPosition) / max(0.001f, abs(dot(parallelogramNormalizedNormal, -lightDirection)));
                        // const float pdf = 1.0f;
                        parallelogramLightContribution += float4(singleParallelogramLightContribution.rgb / pdf, singleParallelogramLightContribution.a);
                    }
                }
            }
        }
    }

    float4 pointLightContribution = float4(0, 0, 0, 0);
    {
        if (payload.Depth > 0)
        {
            float weights[64] = { 0.0f, };
            float totalWeight = 0.0f;
            for(uint pointLightIndex = 0; pointLightIndex < gSceneConstantBuffer.PointLightInfosCount; ++pointLightIndex)
            {
                const float3 lightPosition = gPointLightInfos[pointLightIndex].Positions;
                const float3 lightToHitPosition = lightPosition - hitPosition;
                const float distanceSqr = dot(lightToHitPosition, lightToHitPosition);
                const float3 lightDirection = normalize(lightToHitPosition);
                float fNDotL = max(0.0f, dot(lightDirection, normal));
                weights[pointLightIndex] = fNDotL / distanceSqr;
                totalWeight += weights[pointLightIndex];
            }

            // Sample an index using the weights (roulette selection)
            float rnd = CanonicalSample(pixel, FrameIndex, PrimitiveIndex() + 10);
            float accum = 0.0f;
            uint randomPointLightInfoIndex = 0;
            for(uint pointLightIndex = 0; pointLightIndex < gSceneConstantBuffer.PointLightInfosCount; ++pointLightIndex)
            {
                accum += weights[pointLightIndex] / totalWeight;
                if(rnd < accum)
                {
                    randomPointLightInfoIndex = pointLightIndex;
                    break;
                }
            }
            const float3 lightPosition = gPointLightInfos[randomPointLightInfoIndex].Positions;
            const float3 lightToHitPosition = lightPosition - hitPosition;
            const float distanceSqr = dot(lightToHitPosition, lightToHitPosition);
            const float3 lightDirection = normalize(lightToHitPosition);
            const float3 lightColor = gPointLightInfos[randomPointLightInfoIndex].Color;
            const float radius = gPointLightInfos[randomPointLightInfoIndex].Radius;

            RayDesc ray;
            ray.Origin = hitPosition;
            ray.Direction = lightDirection;
            // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
            // TMin should be kept small to prevent missing geometry at close contact areas.
            ray.TMin = 0.001;
            ray.TMax = 10000.0;
            RayPayload shadowRayPayload;
            shadowRayPayload.Color = float4(0, 0, 0, 0);
            shadowRayPayload.RayT = 0.0f;
            shadowRayPayload.PrimitiveIndex = 0xFFFFFFFF;
            shadowRayPayload.Depth = 0; // Only check for visibility
            TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, shadowRayPayload);
            if (shadowRayPayload.PrimitiveIndex == 0xFFFFFFFF || shadowRayPayload.RayT * shadowRayPayload.RayT > distanceSqr)
            {
                // PDF for weighted sampling
                const float pdf = weights[randomPointLightInfoIndex] / totalWeight;
                const float4 singlePointLightContribution = EvaluateRadiance(lightToHitPosition, lightColor, radius, normal);
                pointLightContribution += float4(singlePointLightContribution.rgb / pdf, singlePointLightContribution.a);
            }
        }
    }

    const uint indirectRaysCount = 1;
    float4 indirectIlluminationContribution = float4(0, 0, 0, 0);
    {
        if (payload.Depth > 0)
        {
            float3 lightColor = float3(0, 0, 0);

            for (uint rayIndex = 0; rayIndex < indirectRaysCount; ++rayIndex)
            {
                const float u1 = CanonicalSample(pixel, FrameIndex, PrimitiveIndex() + 2);
                const float u2 = CanonicalSample(pixel, FrameIndex, PrimitiveIndex() + 3);

                const float3 sample = SampleHemisphereCosineWeighted(float2(u1, u2));

                // Create an orthonormal basis (n, t, b)
                const float3 n = normalize(normal);
                const float3 t = (fabs(normal.x) > 0.1f ? float3(0,1,0) : normalize(cross(float3(1,0,0), n)));
                const float3 b = cross(n, t);

                const float3 lightDirection = (t * sample.x) + (b * sample.y) + (n * sample.z);
                const float radius = 0.0f;

                RayDesc ray;
                ray.Origin = hitPosition;
                ray.Direction = lightDirection;
                // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
                // TMin should be kept small to prevent missing geometry at close contact areas.
                ray.TMin = 0.001;
                ray.TMax = 10000.0;
                RayPayload shadowRayPayload;
                shadowRayPayload.Color = float4(0, 0, 0, 0);
                shadowRayPayload.RayT = 0.0f;
                shadowRayPayload.PrimitiveIndex = 0xFFFFFFFF;
                shadowRayPayload.Depth = payload.Depth - 1;
                TraceRay(gScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, shadowRayPayload);
                if (shadowRayPayload.PrimitiveIndex != 0xFFFFFFFF)
                {
                    const uint isEmissive = IsEmissive(shadowRayPayload.PrimitiveIndex);
                    if (isEmissive == 0)
                    {
                        lightColor = shadowRayPayload.Color.rgb;
                        const float3 lightToHitPosition = lightDirection * shadowRayPayload.RayT;
                        float4 singleIndirectLightContribution = EvaluateRadiance(lightToHitPosition, lightColor, radius, normal);
                        float fNDotL = max(0.0f, dot(lightDirection, normal));
                        const float pdf = fNDotL / PI;
                        indirectIlluminationContribution += float4(singleIndirectLightContribution.rgb / pdf, singleIndirectLightContribution.a);
                    }
                }
            }
        }
    }

    // const uint totalRaysCount = parallelogramLightRaysCount + 1 + indirectRaysCount;
    // const float4 lightContribution = (parallelogramLightContribution + pointLightContribution + indirectIlluminationContribution) / (float)totalRaysCount;
    // const float4 lightContribution = parallelogramLightContribution * ((float)gSceneConstantBuffer.ParallelogramAreaLightInfosCount / (float)parallelogramLightRaysCount);
    // const float4 lightContribution = float4(indirectIlluminationContribution.rgb * (float)indirectRaysCount, indirectIlluminationContribution.a);
    const float4 lightContribution =
        // parallelogramLightContribution * ((float)gSceneConstantBuffer.ParallelogramAreaLightInfosCount / (float)parallelogramLightRaysCount)
            + pointLightContribution
        // + float4(indirectIlluminationContribution.rgb / (float)indirectRaysCount, indirectIlluminationContribution.a)
        ;

    const float3 baseColor = gColors[PrimitiveIndex()];
    return float4(baseColor, 1.0f) * lightContribution;
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

    // Get the base index of the triangle's first 16 bit index.
    const uint indexSizeInBytes = 2;
    const uint indicesPerTriangle = 3;
    const uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    const uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);
    
    const uint isEmissiveBaseIndex = PrimitiveIndex();
    const uint isEmissive = IsEmissive(isEmissiveBaseIndex);

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
    const float3 triangleNormal = HitAttribute(vertexNormals, attr);

    float4 color = ComputeRenderingEquation(payload, hitPosition, triangleNormal);

    if(isEmissive == 1)
    {
        color.xyz += gColors[PrimitiveIndex()];
    }

    payload.Color = color;
    payload.RayT = RayTCurrent();
    payload.PrimitiveIndex = PrimitiveIndex();
}

[shader("miss")]
void MissMain(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.0f, 5.0f, 1.0f);
    payload.Color = background;
    payload.RayT = FLT_MAX;
    payload.PrimitiveIndex = 0xFFFFFFFF;
}