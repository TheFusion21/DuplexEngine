#include "renderer.h"

using namespace DUPLEX_NS_GRAPHICS;

Renderer* Renderer::mpInstance = nullptr;

ui32 Renderer::PixelSizeFromTextureFormat(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::RGBAFLOAT:
		return sizeof(float) * 4U;
	case TextureFormat::RGBFLOAT:
		return sizeof(float) * 3U;

	case TextureFormat::RGBA32:
		return sizeof(byte) * 4U;
	case TextureFormat::RGBA64:
		return sizeof(unsigned short) * 4U;

	case TextureFormat::ALPHA8:
		return sizeof(byte);

	case TextureFormat::RED8:
		return sizeof(byte);
	case TextureFormat::RED16:
		return sizeof(unsigned short);
	case TextureFormat::REDFLOAT:
		return sizeof(float);
	case TextureFormat::RED1:
		return sizeof(bool);

	case TextureFormat::D32:
		return sizeof(byte);

	case TextureFormat::RGFLOAT:
		return sizeof(float) * 2U;
	case TextureFormat::RG32:
		return sizeof(unsigned short) * 2U;
	}
	return 0;
}
