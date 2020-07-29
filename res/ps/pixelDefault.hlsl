#include "../structures.hlsl"

float4 PS_Main(ps_in input) : SV_TARGET
{
    float3 lightDir = float3(1, -1, -1);
    lightDir = normalize(-lightDir);

    float3 view = normalize(eye - (float3)input.positionW);


    float3 Halfway = normalize(lightDir + view);

    float4 Ia = m.Ka * ambientLight;

    float4 Id = m.Kd * saturate(dot(input.normal, lightDir));

    float4 Is = m.Ks * pow(saturate(dot(Halfway, input.normal)), m.Ns);

    float4 lightColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    float4 output = (saturate(Ia + Id) + Is) * lightColor;
    output.a = 1.0f;
    return output;
}