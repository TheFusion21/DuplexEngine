#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "utils/color.h"

struct Material
{
	//Ambient color
	Engine::Utils::FloatColor Ka = { 1.0f, 1.0f, 1.0f, 1.0f }; //4*4 bytes
	//Diffuse Color
	Engine::Utils::FloatColor Kd = { 0.8f, 0.8f, 0.8f, 1.0f }; //4*4 bytes
	//Specular color
	Engine::Utils::FloatColor Ks = { 1.0f, 1.0f, 1.0f, 1.0f }; //4*4 bytes
	//Roughness -> Ns = (1-Roughness) * 900
	float roughness = 0.0f; //1 * 4 bytes
};