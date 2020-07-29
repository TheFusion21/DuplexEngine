#define MAX_LIGHTS 256

struct vs_in
{
    float4 position : POSITION0;
    float4 color : COLOR0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct ps_in
{
    float4 position : SV_POSITION;
    float4 positionW : POSITION0;
    float4 color : COLOR0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct Light
{
    float3 strength;
    float falloffStart;
    float3 direction;
    float fallOffEnd;
    float3 position;
    float spotPower;
};

struct Material
{
    float4 Ka;
    float4 Kd;
    float4 Ks;
    float Ns;
};
cbuffer modelConstant : register(b0)
{
    float4x4 world;
    Material m;
};
cbuffer worldConstant : register(b1)
{
    float4x4 viewProj;
    float3 eye;
    float ambientLight;
};