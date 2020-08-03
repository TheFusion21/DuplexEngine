#pragma once
#include "utils/color.h"
#include "math/types.h"
namespace Engine::Graphics
{
	enum class LightType
	{
		Spot,
		Directional,
		Point
	};
	struct Light
	{
		static const ui32 maxLightCount;

		Engine::Utils::FloatColor color = Engine::Utils::FloatColor::white;
		LightType type = LightType::Directional;
		float strength = 1.0f;

		float angle = 45.0f;
		float range = 10.0f;
	};
}