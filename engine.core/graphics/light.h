#pragma once
#include "utils/color.h"
#include "math/types.h"
namespace DUPLEX_NS_GRAPHICS
{
	
	struct Light
	{
		enum class LightType
		{
			Spot,
			Directional,
			Point
		};
		LightType type = LightType::Directional;
		//Emission

		DUPLEX_NS_UTIL::FloatColor color = DUPLEX_NS_UTIL::FloatColor::white;
		float intensity = 1.0f;
		float indirectMultiplier = 1.0f;

		float angularDiameter = 0.0f;

		float outerAngle  = 30.0f;
		float innerAnglePercent = 0.0f;

		float radius = 0.025f;
		float range = 10;
		static ui32 GetTypeID()
		{
			return 3;
		}
	};
}