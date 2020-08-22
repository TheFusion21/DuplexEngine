#include "../structures.hlsl"

ps_in VS_Main(vs_in input)
{
    ps_in output;
    output.positionW = mul(float4(input.position, 1.0), world).xyz;
    output.position = mul(float4(output.positionW, 1.0), viewProj);

    
    output.normal = normalize(mul(normalize(input.normal), (float3x3)world));

    float3 N = output.normal;
    float3 T = normalize(mul(normalize(input.tangent), (float3x3)world));
    float3 B = normalize(cross(N, T));
    float3x3 TBN = transpose(float3x3(T, B, N));
    output.tangentBasis = TBN;
    output.texCoord = input.texCoord;
    return output;
}