#include "../structures.hlsl"


Texture2D _BaseColor : register(t0);
Texture2D _Metallic : register(t1);
Texture2D _Occlusion : register(t2);
Texture2D _Roughness : register(t3);
Texture2D _NormalMap : register(t4);
Texture2D _EmissiveColorMap : register(t5);
Texture2D textureBRDFLUT : register(t3);

SamplerState defaultSampler : register(s0);
SamplerState spBRDF_Sampler : register(s1);

static const float PI = 3.14159265359;

float3 Uncharted2Tonemap(float3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
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
float3 F_SchlickR(float cosTheta, float3 F0, float roughness)
{
	return F0 + (max((1.0 - roughness).xxx, F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
	float3 tangentNormal = _NormalMap.Sample(defaultSampler, input.texCoord).xyz * 2.0 - 1.0;
	
	return normalize(mul(input.tangentBasis, tangentNormal));
}
float4 PS_Main(ps_in input) : SV_TARGET
{
	// Sample input textures to get shading model params.
	Mat m;
	m.albedo = _BaseColor.Sample(defaultSampler, input.texCoord).rgb;
	m.metalness = _Metallic.Sample(defaultSampler, input.texCoord).r;
	m.roughness = _Roughness.Sample(defaultSampler, input.texCoord).r;

	float3 N = calculateNormal(input);
	float3 view = normalize(eye - input.positionW);

	//Pre calc once per pixel not per light or some shit
	float dotNV = clamp(dot(N, view), 0.0, 1.0);
	
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), m.albedo, m.metalness);


	float3 Lo = float3(0.0f, 0.0f, 0.0f);
	//[unroll(MAX_LIGHTS)]
	for (unsigned int i = 0; i < lightCount; i++)
	{
		if (lights[i].type == 0)
		{
			Lo += LightCalcSpot(i, input.normal, view, m, F0);
		}
		else if (lights[i].type == 1)
		{
			float3 L = normalize(mul(float4(1.0f, 0.0f, 0.0f, 1.0f), lights[i].transform)).xyz;
			float3 H = normalize(view + L);
			float dotNL = clamp(dot(N, L), 0.0, 1.0);
			float dotNH = clamp(dot(N, H), 0.0, 1.0);
			if (dotNL > 0.0)
			{
				// D = Normal distribution (Distribution of the microfacets)
				float D = D_GGX(dotNH, m.roughness);
				// G = Geometric shadowing term (Microfacets shadowing)
				float G = G_SchlicksmithGGX(dotNL, dotNV, m.roughness);
				// F = Fresnel factor (Reflectance depending on angle of incidence)
				float3 F = F_Schlick(dotNV, F0);

				float3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
				float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - m.metalness);

				Lo += (kD * m.albedo / PI + spec) * dotNL * lights[i].color;
			}
		}
		else if (lights[i].type == 2)
			Lo += LightCalcPoint(i, N, view, m, F0);
	}
	//float3 specular = reflection * (F * brdf.x + brdf.y);
	//float F = F_SchlickR(max(dotNV, 0.0), F0, m.roughness);
	//float3 kD = 1.0 - F;
	//float3 ambient = (kD * m.albedo + m.)
	float3 color = m.albedo * 0.1f;
	color += Lo;
	color += _EmissiveColorMap.Sample(defaultSampler, input.texCoord).rgb;
	//TODO: 1.0 to constant buffer variable
	color = Uncharted2Tonemap(color * 1.0);
	color = color * (1.0f / Uncharted2Tonemap((11.2f).xxx));
	//Gama correction
	//TODO: change float3 to constant buffer variable
	color = pow(color, float3(0.4545, 0.4545, 0.4545));
	return float4(color, 1.0f);
}