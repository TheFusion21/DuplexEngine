#include "../structures.hlsl"


ps_in VS_Main(vs_in input)
{
    ps_in output;
    output.positionW = mul(input.position, world);
    output.position = mul(output.positionW, viewProj);

    output.color = input.color;

    output.normal = normalize(mul(normalize(input.normal), (float3x3)world));
    output.texCoord = input.texCoord;
    return output;
}