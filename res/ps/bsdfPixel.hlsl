#include "../structures.hlsl"


Texture2D albedoTexture : register(t0);
Texture2D metalnessTexture : register(t1);
Texture2D ambientOcculsionTexture : register(t2);
Texture2D roughnessTexture : register(t3);
Texture2D normalTexture : register(t4);
Texture2D emissiveTexture : register(t5);


SamplerState defaultSampler : register(s0);
SamplerState spBRDF_Sampler : register(s1);

static const float PI = 3.14159265359;

// Normal Distribution function
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (PI * denom * denom);
}
// Geometric Shadowing function
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}
// Fresnel function
float3 F_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float3 BRDF(float3 lightColor, float3 L, float3 V, float3 N, Mat m, float3 F0)
{
	float3 H = normalize(V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);


	

	float3 color = float3(0.0f, 0.0f, 0.0f);

	if (dotNL > 0.0)
	{
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, m.roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, m.roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNV, F0);

		float3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001f);
		float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - m.metalness);

		color += (kD * m.albedo / PI + spec) * dotNL * lightColor;
	}
	return color;
}
float3 LightCalcDir(int i, float3 N, float3 V, Mat m, float3 L, float3 F0)
{
	return BRDF(lights[i].color, L, V, N, m, F0) * lights[i].intensity;
}
float3 LightCalcSpot(int i, float3 N, float3 V, Mat m, float3 F0)
{
	return float3(0.0f, 0.0f, 0.0f);
}
float3 LightCalcPoint(int i, float3 N, float3 V, Mat m, float3 F0)
{
	return float3(0.0f, 0.0f, 0.0f);
}

float3 calculateNormal(ps_in input)
{
	float3 tangentNormal = normalTexture.Sample(defaultSampler, input.texCoord).xyz * 2.0 - 1.0;
	
	return normalize(mul(input.tangentBasis, tangentNormal));
}
float4 PS_Main(ps_in input) : SV_TARGET
{
	// Sample input textures to get shading model params.

	Mat m;
	m.albedo = albedoTexture.Sample(defaultSampler, input.texCoord).rgb;
	m.metalness = metalnessTexture.Sample(defaultSampler, input.texCoord).r;
	m.roughness = roughnessTexture.Sample(defaultSampler, input.texCoord).r;
	float3 N = calculateNormal(input);
	float3 view = normalize(eye - input.positionW);
	float3 color = m.albedo * 0.1f;
	float3 Lo = float3(0.0f, 0.0f, 0.0f);

	//Pre calc once per pixel not per light or some shit
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, m.albedo, m.metalness);

	[unroll(MAX_LIGHTS)]
	for (unsigned int i = 0; i < lightCount; i++)
	{
		if (lights[i].type == 0)
		{
			Lo += LightCalcSpot(i, input.normal, view, m, F0);
		}
		else if (lights[i].type == 1)
		{
			float3 L = normalize(mul(float4(1.0f, 0.0f, 0.0f, 1.0f), lights[i].transform)).xyz;
			Lo += LightCalcDir(i, N, view, m, L, F0);
		}
		else if (lights[i].type == 2)
			Lo += LightCalcPoint(i, N, view, m, F0);
	}
	color += Lo;
	color = pow(color, float3(0.4545, 0.4545, 0.4545));
	color += emissiveTexture.Sample(defaultSampler, input.texCoord).rgb;
	return float4(color, 1.0f);
}