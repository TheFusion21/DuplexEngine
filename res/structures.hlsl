#define MAX_LIGHTS 8

struct vs_in
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT0;
    float3 bittangent : BITTANGENT;
};

struct ps_in
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3x3 tangentBasis : TBASIS;
};

struct Light
{
    unsigned int type;
    float3 color;
    float intensity;
    float indirectMul;

    float angularDiameter;
    float outerAngle;
    float innerAngle;
    float radius;
    float range;

    float4x4 transform;
    float3 position;
};

cbuffer modelConstant : register(b0)
{
    float4x4 world;
};
cbuffer worldConstant : register(b1)
{
    Light lights[MAX_LIGHTS];
    float4x4 viewProj;
    float3 eye;
    unsigned int lightCount;
    
};

struct Mat
{
    float3 albedo;
    float metalness;
    float roughness;
};