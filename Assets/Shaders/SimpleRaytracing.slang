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

float4 
ComputeRenderingEquation(inout RayPayload payload, const float3 hitPosition, const float3 normal)
{
    const uint2 pixel = DispatchRaysIndex().xy;

    // Emissive radiance
    float3 emissiveRadiance = float3(0.0f, 0.0f, 0.0f);
    if (IsEmissive(PrimitiveIndex()) == 1)
    {
        const float tempEmissiveStrength = 100.0f;
        emissiveRadiance = gColors[PrimitiveIndex()] * tempEmissiveStrength;
    }
    
    // Integral
    float3 monteCarloEstimate = float3(0.0f, 0.0f, 0.0f);
    if (payload.Depth > 0)
    {
        // BRDF. Lambertian diffuse BRDF is constant, so we can take it out of the integral.
        // Note: For other BRDFs, the BRDF would need to be evaluated per sample.
        const float3 brdf = CalculateLambertianDiffuseBrdf(PrimitiveIndex());
        const uint HEMISPHERE_SAMPLES_COUNT = 4;
        float3 monteCarloIntegrandSum = float3(0.0f, 0.0f, 0.0f);
        for(uint sampleIndex = 0; sampleIndex < HEMISPHERE_SAMPLES_COUNT; ++sampleIndex)
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
            const float3 integrand = incomingRadiance;
            monteCarloIntegrandSum += integrand;
        }
        monteCarloEstimate = (monteCarloIntegrandSum * brdf) / HEMISPHERE_SAMPLES_COUNT;
    }

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