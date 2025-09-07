cbuffer Camera
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

cbuffer EmissiveInfo
{
    float3 EmissiveGeometryPosition;
    float3 EmissiveGeometryColor;
};

cbuffer PushConstants
{
    float4 Color;
};

struct VSInput
{
    float3	Position : POSITION;
    float3	Normal   : NORMAL;
};

struct FSOutput
{
    float4 Color : SV_Target;
};

// Vertex  Shader

struct VSOutput
{
    float4 SsPosition : SV_Position;
    float3 WsPosition : TEXCOORD0;
    float3 Normal   : NORMAL;
};

[shader("vertex")]
VSOutput VSMain(VSInput vsInput)
{
    VSOutput output;

    float3 position = vsInput.Position;
    float3 normal   = vsInput.Normal;

    output.SsPosition = mul(ViewMatrix, float4(position, 1.0));
    output.SsPosition = mul(ProjectionMatrix, output.SsPosition);
    output.WsPosition = position;
    output.Normal   = normal;

    return output;
}

// Fragment Shader

[shader("fragment")]
FSOutput FSMain(VSOutput vsOutput)
{
    FSOutput output;
    output.Color = Color;
    
    const float3 emissiveDirection = normalize(EmissiveGeometryPosition - vsOutput.WsPosition);
    constexpr float intensityOfIncomingLight = 1.0f;
    const float3 lambertian = dot(emissiveDirection, vsOutput.Normal) * EmissiveGeometryColor * intensityOfIncomingLight;
    const float3 radiance = lambertian;
    output.Color.rgb *= radiance;
    return output;
}
