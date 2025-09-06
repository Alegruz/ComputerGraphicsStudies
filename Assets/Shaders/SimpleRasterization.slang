cbuffer Camera
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

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
    output.Normal   = normal;

    return output;
}

// Fragment Shader

[shader("fragment")]
FSOutput FSMain(VSOutput vsOutput)
{
    const float3 normal = (vsOutput.Normal + 1.0f) * 0.5f;

    FSOutput output;
    output.Color = float4(normal, 1.0);
    return output;
}
