#include "../structures.hlsl"

Texture2D g_texture;

SamplerState textureSampler;
static const float threshold = 1.0f/8.f;
static const float bias = 0.01f;


float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float4 PS_Main(ps_in input) : SV_TARGET
{
    float3 color = g_texture.Sample(textureSampler, input.texCoord).rgb;
    
    float signedDist = median(color.r, color.g, color.b);
    
    float d = smoothstep(0.5f - threshold, 0.5f + threshold, signedDist);
    
    if (d < bias)
        discard;

    return float4(0.0, 0.0, 0.0, d);
}