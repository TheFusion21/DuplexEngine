#include "color.h"
#include <cmath>

using namespace DUPLEX_NS_UTIL;

const FloatColor FloatColor::black = { 0.0f, 0.0f, 0.0f, 1.0f };
const FloatColor FloatColor::grey = { 0.5f, 0.5f, 0.5f, 1.0f };
const FloatColor FloatColor::white = { 1.0f, 1.0f, 1.0f, 1.0f };
const FloatColor FloatColor::red = { 1.0f, 0.0f, 0.0f, 1.0f };
const FloatColor FloatColor::blue = { 0.0f, 0.0f, 1.0f, 1.0f };
const FloatColor FloatColor::green = { 0.0f, 1.0f, 0.0f, 1.0f };
const FloatColor FloatColor::yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
const FloatColor FloatColor::orange = { 1.0f, 0.64453125f, 0.0f, 1.0f };
const FloatColor FloatColor::magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
const FloatColor FloatColor::aqua = { 0.0f, 1.0f, 1.0f, 1.0f };

FloatColor FloatColor::FromTemperature(int kelvin)
{
	float temp = floorf(kelvin / 100.0f);

	float r, g, b;

	if (temp < 66.0f)
	{
		r = 255.0f;

		g = temp;
		g = 99.4708025861f * logf(g) - 161.1195681661f;

		if (temp <= 19.0f)
		{
			b = 0.0f;
		}
		else
		{
			b = temp - 10.0f;
			b = 138.5177312231f * logf(b) - 305.0447927307f;
		}
	}
	else
	{
		r = temp - 60.0f;
		r = 329.698727446f * powf(r, -0.1332047592f);

		g = temp - 60.0f;
		g = 288.1221695283f * powf(g, -0.0755148492f);

		b = 255.0f;
	}

	return { r / 255.0f, g / 255.0f, b / 255.0f, 1.0f };
}