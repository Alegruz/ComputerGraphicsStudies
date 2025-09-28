#define FLT_MAX         (3.402823466e+38)
#define PI              (3.1415926535897f)

#define PARALLELOGRAM_AREA_LIGHT_SAMPLES_COUNT (1)
#define POINT_LIGHT_SAMPLES_COUNT (1)
#define HEMISPHERE_SAMPLES_COUNT (0)
#define RESAMPLE_COUNT (8)
#define SPATIAL_NEIGHBORHOOD_RADIUS_RATE (1.0f / 128.0f)
#define SPATIAL_NEIGHBORS_COUNT (1)

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
    uint seed = pixel.x * 73856093u                                 // big prime
                ^ pixel.y * 19349663u                               // another big prime
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

// ReSTIR paper algorithm 2: Weighted reservoir sampling.
void
InitializeParallelogramAreaLightReservoir(out ParallelogramAreaLightReservoir reservoir)
{
    // y ← 0.   The output sample
    // w_sum ← 0.   The sum of weights
    // M ← 0.   The number of samples seen so far
    reservoir = (ParallelogramAreaLightReservoir)0;
}

void
InitializePointLightReservoir(out PointLightReservoir reservoir)
{
    reservoir = PointLightReservoir(0);
}

void
InitializeIndirectLightReservoir(out IndirectLightReservoir reservoir)
{
    reservoir = IndirectLightReservoir(0);
}

void
UpdateParallelogramAreaLightReservoir(inout ParallelogramAreaLightReservoir inoutReservoir, const ParallelogramAreaLightReservoir reservoir, const uint2 pixel, const uint bufferIndexByPixel)
{
    const float weight = ConvertColorToLuminance(reservoir.Weight * reservoir.Pdf) * reservoir.SamplesCount;
    inoutReservoir.WeightSum += weight;
    inoutReservoir.SamplesCount += reservoir.SamplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + reservoir.SamplesCount * bufferIndexByPixel * 17, 4);
    const float probability = weight / inoutReservoir.WeightSum;
    if (canonicalSample < probability)
    {
        inoutReservoir.ParallelogramAreaLightPositionSample = reservoir.ParallelogramAreaLightPositionSample;
        inoutReservoir.ParallelogramAreaLightNormalSample = reservoir.ParallelogramAreaLightNormalSample;
        inoutReservoir.Pdf = reservoir.Pdf;
    }
}

void
UpdatePointLightReservoir(inout PointLightReservoir inoutReservoir, const PointLightReservoir reservoir, const uint2 pixel, const uint bufferIndexByPixel)
{
    const float weight = ConvertColorToLuminance(reservoir.Weight * reservoir.Pdf) * reservoir.SamplesCount;
    inoutReservoir.WeightSum += weight;
    inoutReservoir.SamplesCount += reservoir.SamplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + reservoir.SamplesCount * bufferIndexByPixel * 23, 4);
    const float probability = weight / inoutReservoir.WeightSum;
    if (canonicalSample < probability)
    {
        inoutReservoir.LightIndexSample = reservoir.LightIndexSample;
        inoutReservoir.Pdf = reservoir.Pdf;
    }
}

void
UpdateIndirectLightReservoir(inout IndirectLightReservoir inoutReservoir, const IndirectLightReservoir reservoir, const uint2 pixel, const uint bufferIndexByPixel)
{
    const float weight = ConvertColorToLuminance(reservoir.Weight * reservoir.Pdf) * reservoir.SamplesCount;
    inoutReservoir.WeightSum += weight;
    inoutReservoir.SamplesCount += reservoir.SamplesCount;
    const float canonicalSample = CanonicalSample(pixel, FrameIndex + reservoir.SamplesCount * 17, 4);
    const float probability = weight / inoutReservoir.WeightSum;
    if (canonicalSample < probability)
    {
        inoutReservoir.Direction = reservoir.Direction;
        inoutReservoir.Pdf = reservoir.Pdf;
    }
}

float
ParallelogramAreaLightDesiredPdf(const ParallelogramAreaLightInfo lightInfo, const ParallelogramAreaLightReservoir otherReservoir, const float3 position, const float3 normal)
{
    const float3 incomingRadiance = lightInfo.Color.rgb * 40.0f;

    const float3 parallelogramAreaLightPosition = otherReservoir.ParallelogramAreaLightPositionSample;
    const float3 hitPositionToParallelogramAreaLightPosition = parallelogramAreaLightPosition - position;
    const float distanceSquared = dot(hitPositionToParallelogramAreaLightPosition, hitPositionToParallelogramAreaLightPosition);
    const float distance = sqrt(distanceSquared);
    const float3 worldSampleDir = normalize(parallelogramAreaLightPosition - position);
    const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
    const float3 triangleNormal = otherReservoir.ParallelogramAreaLightNormalSample;
    const float cosAtLight = max(0.0f, dot(-worldSampleDir, triangleNormal));
    const float geometryTerm = cosAtSurface * cosAtLight / distanceSquared;
    const float desiredPdf = ConvertColorToLuminance(incomingRadiance * geometryTerm);
    return desiredPdf;
}

void
CombineParallelogramAreaLightReservoirs(inout ParallelogramAreaLightReservoir inoutReservoir, const uint2 pixel0, const uint2 pixel1, const uint bufferIndexByPixel0, const uint bufferIndexByPixel1, const float3 position, const float3 normal)
{
    InitializeParallelogramAreaLightReservoir(inoutReservoir);
    UpdateParallelogramAreaLightReservoir(inoutReservoir, gParallelogramAreaLightReservoir[bufferIndexByPixel0], pixel0, bufferIndexByPixel0);

    ParallelogramAreaLightReservoir otherReservoir = gPrevParallelogramAreaLightReservoir[bufferIndexByPixel1];
    ParallelogramAreaLightInfo lightInfo = gParallelogramAreaLightInfos[0];
    const float3 incomingRadiance = lightInfo.Color.rgb * 40.0f;

    const float otherPdf = otherReservoir.Pdf;
    otherReservoir.Pdf = ParallelogramAreaLightDesiredPdf(lightInfo, otherReservoir, position, normal);

    UpdateParallelogramAreaLightReservoir(inoutReservoir, gPrevParallelogramAreaLightReservoir[bufferIndexByPixel1], pixel1, bufferIndexByPixel1);
    otherReservoir.Pdf = otherPdf;

    inoutReservoir.Pdf = ParallelogramAreaLightDesiredPdf(lightInfo, inoutReservoir, position, normal);
    const float dividend = inoutReservoir.Pdf * inoutReservoir.SamplesCount;
    inoutReservoir.Weight = dividend == 0.0f ? 0.0f : inoutReservoir.WeightSum / dividend;
}

float
PointLightDesiredPdf(const PointLightInfo lightInfo, const PointLightReservoir otherReservoir, const float3 position, const float3 normal)
{
    const uint pointLightIndex = otherReservoir.LightIndexSample;
    const float pointLightStrength = 1000.0f;
    const float3 incomingRadiance = gPointLightInfos[pointLightIndex].Color * pointLightStrength;

    const float3 pointLightPosition = gPointLightInfos[pointLightIndex].Positions;
    const float3 hitPositionToPointLightPosition = pointLightPosition - position;
    const float distanceSquared = dot(hitPositionToPointLightPosition, hitPositionToPointLightPosition);
    const float3 worldSampleDir = normalize(pointLightPosition - position);

    const float geometryTerm = dot(worldSampleDir, normal) / distanceSquared;
    const float3 integrand = incomingRadiance * geometryTerm;
    const float desiredPdf = ConvertColorToLuminance(integrand);
    return desiredPdf;
}

void
CombinePointLightReservoirs(inout PointLightReservoir inoutReservoir, const uint2 pixel0, const uint2 pixel1, const uint bufferIndexByPixel0, const uint bufferIndexByPixel1, const float3 position, const float3 normal)
{
    InitializePointLightReservoir(inoutReservoir);
    UpdatePointLightReservoir(inoutReservoir, gPointLightReservoir[bufferIndexByPixel0], pixel0, bufferIndexByPixel0);

    PointLightReservoir otherReservoir = gPrevPointLightReservoir[bufferIndexByPixel1];
    PointLightInfo lightInfo = gPointLightInfos[0];
    const float3 incomingRadiance = lightInfo.Color.rgb * 40.0f;

    const float otherPdf = otherReservoir.Pdf;
    otherReservoir.Pdf = PointLightDesiredPdf(lightInfo, otherReservoir, position, normal);

    UpdatePointLightReservoir(inoutReservoir, gPrevPointLightReservoir[bufferIndexByPixel1], pixel1, bufferIndexByPixel1);
    otherReservoir.Pdf = otherPdf;

    inoutReservoir.Pdf = PointLightDesiredPdf(lightInfo, inoutReservoir, position, normal);
    const float dividend = inoutReservoir.Pdf * inoutReservoir.SamplesCount;
    inoutReservoir.Weight = dividend == 0.0f ? 0.0f : inoutReservoir.WeightSum / dividend;
}

float
IndirectLightDesiredPdf(const IndirectLightReservoir otherReservoir, const float3 normal)
{
    const float3 worldSampleDir = otherReservoir.Direction;

    const float cosAtSurface = max(0.0f, dot(worldSampleDir, normal));
    const float desiredPdf = cosAtSurface;
    return desiredPdf;
}

void
CombineIndirectLightReservoirs(inout IndirectLightReservoir inoutReservoir, const uint2 pixel0, const uint2 pixel1, const uint bufferIndexByPixel0, const uint bufferIndexByPixel1, const float3 position, const float3 normal)
{
    InitializeIndirectLightReservoir(inoutReservoir);
    UpdateIndirectLightReservoir(inoutReservoir, gIndirectLightReservoir[bufferIndexByPixel0], pixel0, bufferIndexByPixel0);

    IndirectLightReservoir otherReservoir = gPrevIndirectLightReservoir[bufferIndexByPixel1];
    const float otherPdf = otherReservoir.Pdf;
    otherReservoir.Pdf = IndirectLightDesiredPdf(otherReservoir, normal);

    UpdateIndirectLightReservoir(inoutReservoir, gPrevIndirectLightReservoir[bufferIndexByPixel1], pixel1, bufferIndexByPixel1);
    otherReservoir.Pdf = otherPdf;

    inoutReservoir.Pdf = IndirectLightDesiredPdf(inoutReservoir, normal);
    const float dividend = inoutReservoir.Pdf * inoutReservoir.SamplesCount;
    inoutReservoir.Weight = dividend == 0.0f ? 0.0f : inoutReservoir.WeightSum / dividend;
}