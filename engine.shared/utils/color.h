#pragma once
namespace Engine::Utils
{
	class FloatColor
	{
	public:
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;

		static const FloatColor black;
		static const FloatColor grey;
		static const FloatColor white;
		static const FloatColor red;
		static const FloatColor blue;
		static const FloatColor green;
		static const FloatColor yellow;
		static const FloatColor orange;
		static const FloatColor magenta;
		static const FloatColor aqua;
	};
}