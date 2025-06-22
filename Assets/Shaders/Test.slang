struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

typedef VSOutput PSInput;

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    return output;
}

[shader("fragment")]
float4 PSMain(PSInput input) : SV_TARGET
{
    // Output a solid color (e.g., red)
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}