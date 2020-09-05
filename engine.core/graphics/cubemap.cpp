#include "cubemap.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION
#include "utils/exceptions.h"
#include "renderer.h"
using namespace Engine::Graphics;
using namespace Engine::Utils;

Cubemap Cubemap::LoadFromFile(const char* filename, bool enableReadWrite)
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
		if (channels == 3)
		{
			channels = 4;
			data = stbi_load_from_file_16(f, &width, &height, nullptr, STBI_rgb_alpha);
		}
		else
		{
			data = stbi_load_from_file_16(f, &width, &height, nullptr, STBI_default);
		}
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
		compSize = sizeof(unsigned short);
		switch (channels)
		{
		case 4:
			format = TextureFormat::RGBA64;
			break;
		case 2:
			format = TextureFormat::RG32;
			break;
		case 1:
			format = TextureFormat::RED16;
			break;
		}
	}
	else if (stbi_is_hdr_from_file(f))
	{
		data = stbi_loadf_from_file(f, &width, &height, nullptr, STBI_default);
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
		compSize = sizeof(float);
		switch (channels)
		{
		case 4:
			format = TextureFormat::RGBAFLOAT;
			break;
		case 3:
			format = TextureFormat::RGBFLOAT;
			break;
		case 2:
			format = TextureFormat::RGFLOAT;
			break;
		case 1:
			format = TextureFormat::REDFLOAT;
			break;
		}
	}
	else
	{
		if (channels == 3)
		{
			channels = 4;
			data = stbi_load_from_file(f, &width, &height, nullptr, STBI_rgb_alpha);
		}
		else
		{
			data = stbi_load_from_file(f, &width, &height, nullptr, STBI_default);
		}
		if (data == nullptr)
		{
			throw new FileLoadException(filename);
		}
	}
	fclose(f);
	//Determine layout of image
	CubemapLayout layout = FromSizes(width, height);
	//Determine facesize of image
	ui32 size = 0;
	switch (layout)
	{
	case CubemapLayout::VerticalCross:
		size = width / 3U;
		break;
	case CubemapLayout::HorizontalCross:
		size = height / 3U;
		break;
	case CubemapLayout::Column:
		size = width;
		break;
	default:
		size = height;
	}

	Cubemap cube(size, layout, enableReadWrite, format);
	cube.data = data;
	cube._texResource = Renderer::GetInstancePtr()->CreateTexture(cube._width, cube._height, 1, cube._format, cube.data);
	cube._texResourceView = Renderer::GetInstancePtr()->CreateCubemapSRV(cube._texResource, cube._format);
	if (!cube.enableReadWrite)
	{
		stbi_image_free(cube.data);
		cube.data = nullptr;
	}
	return cube;
}

Cubemap::~Cubemap()
{
}

Cubemap::Cubemap(ui32 size, CubemapLayout layout, bool enableReadWrite) : Cubemap(size, layout, enableReadWrite, TextureFormat::RGBA32)
{
}

Cubemap::Cubemap(ui32 size, CubemapLayout layout, bool enableReadWrite, TextureFormat format) : Texture(enableReadWrite)
{
	this->_format = format;
	this->_layout = layout;
	switch (layout)
	{
	case CubemapLayout::VerticalCross:
		this->_width = size * 3U;
		this->_height = size * 4U;
		break;
	case CubemapLayout::HorizontalCross:
		this->_width = size * 4U;
		this->_height = size * 3U;
		break;
	case CubemapLayout::Column:
		this->_width = size;
		this->_height = size * 6U;
		break;
	default:
		this->_width = size * 6U;
		this->_height = size;
	}
}

CubemapLayout Engine::Graphics::Cubemap::FromSizes(ui32 width, ui32 height)
{
	//Check for vertical cross
	{
		ui32 fWidth = width / 3U;
		if (fWidth * 4U == height)
			return CubemapLayout::VerticalCross;
	}
	//Check for horizontal cross
	{
		ui32 fHeight = height / 3U;
		if (fHeight * 4U == width)
			return CubemapLayout::HorizontalCross;
	}
	//Check for column based
	{
		if (width * 6U == height)
			return CubemapLayout::Column;
	}

	return CubemapLayout::Row;
}
