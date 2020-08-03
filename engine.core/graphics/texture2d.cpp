#include "texture2d.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION
#include "utils/exceptions.h"
using namespace Engine::Graphics;
using namespace Engine::Utils;


Texture2D Texture2D::blackTexture = Texture2D::FromValue(1, 1, false, 0.0f);
Texture2D Texture2D::whiteTexture = Texture2D::FromValue(1, 1, false, 1.0f);
Texture2D Texture2D::grayTexture = Texture2D::FromValue(1, 1, false, 0.5f);
Texture2D Texture2D::normalTexture = Texture2D::FromColor(1, 1, false, FloatColor{ 0.5f, 0.5f, 1.0f, 1.0f });

Texture2D Engine::Graphics::Texture2D::FromColor(ui32 width, ui32 height, bool enableReadWrite, Engine::Utils::FloatColor color)
{
	Texture2D tex(width, height, enableReadWrite);

	byte* data = new byte [width * height * 4U];

	return tex;
}

Texture2D Engine::Graphics::Texture2D::FromValue(ui32 width, ui32 height, bool enableReadWrite, float value, SingleChannelMode mode)
{
	TextureFormat format;
	switch (mode)
	{
	case SingleChannelMode::ALPHA:
		format = TextureFormat::ALPHA8;
		break;
	case SingleChannelMode::RED:
	default:
		format = TextureFormat::RED8;
		break;
	}
	Texture2D tex(width, height, enableReadWrite, format);

	byte* data = new byte[width * height];

	if (mode == SingleChannelMode::RED)
		tex._format = TextureFormat::RED8;
	else if (mode == SingleChannelMode::ALPHA)
		tex._format = TextureFormat::ALPHA8;

	return tex;
}

Texture2D Engine::Graphics::Texture2D::LoadFromFile(const char* filename, bool enableReadWrite)
{
	FILE* f = fopen(filename, "rb");
	if (!f)
	{
		throw new NoFileException(filename);
	}
	int width = 0, height = 0, channels = 4;
	if (!stbi_info_from_file(f, &width, &height, &channels))
	{
		throw new FileLoadException(filename);
	}
	int forceChannel = STBI_default;
	if (channels == STBI_rgb)
		forceChannel = STBI_rgb_alpha;
	else if(channels == STBI_grey_alpha)
		forceChannel = STBI_rgb_alpha;
	Texture2D tex(width, height, enableReadWrite);

	void* data = nullptr;
	if (stbi_is_16_bit_from_file(f))
	{
		//data is ui16
		data = stbi_load_from_file_16(f, &width, &height, nullptr, forceChannel);
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	else if (stbi_is_hdr_from_file(f))
	{
		data = stbi_loadf_from_file(f, &width, &height, nullptr, forceChannel);
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	else
	{
		//data is byte
		data = stbi_load_from_file(f, &width, &height, nullptr, forceChannel);
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	tex.data = data;
	//tex.data = data;
	tex.pitch = width * sizeof(byte) * channels;


	fclose(f);
	return tex;
}

Texture2D::Texture2D(ui32 width, ui32 height, bool enableReadWrite) : Texture2D(width, height, enableReadWrite, TextureFormat::RGBA32)
{

}

Texture2D::Texture2D(ui32 width, ui32 height, bool enableReadWrite, TextureFormat format) : Texture(enableReadWrite), format(_format)
{
	this->_format = format;
	this->_width = width;
	this->_height = height;
	this->_dimension = TextureDimension::Tex2D;
	switch (this->_format)
	{
	case TextureFormat::RGBA32:
		this->pitch = width * sizeof(byte) * 4U;
		break;
	case TextureFormat::RED1:
	case TextureFormat::RED8:
	case TextureFormat::ALPHA8:
		this->pitch = width * sizeof(byte);
		break;
	case TextureFormat::RGBAFLOAT:
		this->pitch = width * sizeof(float) * 4U;
		break;
	}
}


void Engine::Graphics::Texture2D::Resize(ui32 width, ui32 height)
{

}
