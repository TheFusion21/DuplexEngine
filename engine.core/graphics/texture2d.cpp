#include "texture2d.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION
#include "utils/exceptions.h"
#include "renderer.h"
#include "utils/util.h"
#include "math/mathutils.h"
using namespace Engine::Graphics;
using namespace Engine::Utils;
using namespace Engine::Math;

//Texture2D Texture2D::blackTexture = Texture2D::FromValue(1, 1, false, 0.0f);
//Texture2D Texture2D::whiteTexture = Texture2D::FromValue(1, 1, false, 1.0f);
//Texture2D Texture2D::grayTexture = Texture2D::FromValue(1, 1, false, 0.5f);
//Texture2D Texture2D::normalTexture = Texture2D::FromColor(1, 1, false, FloatColor{ 0.5f, 0.5f, 1.0f, 1.0f });

Texture2D Engine::Graphics::Texture2D::FromColor(ui32 width, ui32 height, bool enableReadWrite, Engine::Utils::FloatColor color)
{
	Texture2D tex(width, height, enableReadWrite, TextureFormat::RGBAFLOAT);

	float* data = static_cast<float*>(tex.data);
	for (ui32 i = 0; i < width * height * 4U;i+=4)
	{
		data[i+0] = color.r;
		data[i+1] = color.g;
		data[i+2] = color.b;
		data[i+3] = color.a;
	}
	tex._texResource = Renderer::GetInstancePtr()->CreateTexture(tex._width, tex._height, 1, tex._format, tex.data);
	tex._texResourceView = Renderer::GetInstancePtr()->CreateTextureSRV(tex._texResource, tex._format);
	Renderer::GetInstancePtr()->ReleaseTexture(tex._texResource);
	return tex;
}

Texture2D Engine::Graphics::Texture2D::FromValue(ui32 width, ui32 height, bool enableReadWrite, float value, SingleChannelMode mode)
{
	value = Clamp(value, 0.0f, 1.0f);
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

	byte* data = static_cast<byte*>(tex.data);
	for (ui32 i = 0; i < width * height; i++)
	{
		data[i + 0] = static_cast<byte>(value * 255);
	}
	tex._texResource = Renderer::GetInstancePtr()->CreateTexture(tex._width, tex._height, 1, tex._format, tex.data);
	tex._texResourceView = Renderer::GetInstancePtr()->CreateTextureSRV(tex._texResource, tex._format);
	Renderer::GetInstancePtr()->ReleaseTexture(tex._texResource);
	return tex;
}

Texture2D Engine::Graphics::Texture2D::LoadFromFile(const char* filename, bool enableReadWrite)
{
	FILE* f = stbi__fopen(filename, "rb");
	if (!f)
	{
		throw new NoFileException(filename);
	}
	int width = 0, height = 0, channels = 4;
	if (!stbi_info_from_file(f, &width, &height, &channels))
	{
		throw new FileLoadException(filename);
	}
	void* data = nullptr;
	size_t compSize = sizeof(byte);
	TextureFormat format = TextureFormat::RGBA32;
	if (stbi_is_16_bit_from_file(f))
	{
		//data is ui16
		data = stbi_load_from_file_16(f, &width, &height, nullptr, STBI_default);
		compSize = sizeof(unsigned short);

		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	else if (stbi_is_hdr_from_file(f))
	{
		data = stbi_loadf_from_file(f, &width, &height, nullptr, STBI_default);
		compSize = sizeof(float);
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
		switch (channels)
		{
		case 4:
			format = TextureFormat::RGBAFLOAT;
			break;
		case 1:
			format = TextureFormat::REDFLOAT;
			break;
		}
	}
	else
	{
		//data is byte
		if (channels == 3)
		{
			channels = 4;
			data = stbi_load_from_file(f, &width, &height, nullptr, STBI_rgb_alpha);
		}
		else
			data = stbi_load_from_file(f, &width, &height, nullptr, STBI_default);

		switch (channels)
		{
		case 4:
			format = TextureFormat::RGBA32;
			break;
		case 1:
			format = TextureFormat::RED8;
			break;
		}
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	fclose(f);
	Texture2D tex(width, height, enableReadWrite, format);
	tex.data = data;
	tex._texResource = Renderer::GetInstancePtr()->CreateTexture(tex._width, tex._height, 1, tex._format, tex.data);
	tex._texResourceView = Renderer::GetInstancePtr()->CreateTextureSRV(tex._texResource, tex._format);
	Renderer::GetInstancePtr()->ReleaseTexture(tex._texResource);
	if (!tex.enableReadWrite)
	{
		stbi_image_free(tex.data);
		tex.data = nullptr;
	}
	return tex;
}

Engine::Graphics::Texture2D::~Texture2D()
{
}

Texture2D::Texture2D(ui32 width, ui32 height, bool enableReadWrite) : Texture2D(width, height, enableReadWrite, TextureFormat::RGBA32)
{

}

Texture2D::Texture2D(ui32 width, ui32 height, bool enableReadWrite, TextureFormat format) : Texture(enableReadWrite)
{
	this->_format = format;
	this->_width = width;
	this->_height = height;
	this->_dimension = TextureDimension::Tex2D;

	ui32 compCount = 0;
	ui32 compSize = 0;
	switch (this->_format)
	{
	case TextureFormat::RED1:
		compCount = 1;
		compSize = sizeof(bool);
		this->data = new bool[width * height];
		break;
	case TextureFormat::RED8:
		compCount = 1;
		compSize = sizeof(byte);
		this->data = new byte[width * height * compCount];
		break;
	case TextureFormat::ALPHA8:
		compCount = 1;
		compCount = sizeof(byte);
		this->data = new byte[width * height * compCount];
		break;
	case TextureFormat::RGBA32:
		compCount = 4;
		compSize = sizeof(byte)* 4U;
		this->data = new byte[width * height * compCount];
		break;
	case TextureFormat::RGBAFLOAT:
		compCount = 4;
		compSize = sizeof(float) * 4U;
		this->data = new float[width * height * compCount];
		break;
	}
	this->pitch = width * compSize;

}
void Engine::Graphics::Texture2D::Resize(ui32 width, ui32 height)
{
	
}