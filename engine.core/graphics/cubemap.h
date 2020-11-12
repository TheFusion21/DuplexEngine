#pragma once

#include "texture.h"
namespace DUPLEX_NS_GRAPHICS
{
	enum class CubemapLayout
	{
		VerticalCross,
		HorizontalCross,
		Column,
		Row
	};
	class Cubemap : public Texture
	{
	public:
		static Cubemap LoadFromFile(const char* filename, bool enableReadWrite);

		~Cubemap();

		Cubemap(ui32 size, CubemapLayout layout, bool enableReadWrite);

		Cubemap(ui32 size, CubemapLayout layout, bool enableReadWrite, TextureFormat format);
	private:
		CubemapLayout _layout;
		static CubemapLayout FromSizes(ui32 width, ui32 height);
	};
}