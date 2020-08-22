#pragma once
#include "math/types.h"
namespace Engine::Graphics
{
	

	enum class TextureDimension
	{
		None,
		Tex2D,
		Tex3D,
		Tex2DArray,
		CubeArray,
		Cube
	};
	
	enum class FilterMode
	{
		Point,
		Bilinear,
		Trilinear
	};
	enum class TextureWrapMode
	{
		Repeat,
		Clamp,
		Mirror
	};
	class Texture
	{
	public:
		static unsigned long currentTextureMemory;

		const ui32& GetWidth() const
		{
			return _width;
		}
		const ui32& GetHeight() const
		{
			return _height;
		}
		const TextureDimension& GetDimension() const
		{
			return _dimension;
		}

		FilterMode filterMode = FilterMode::Bilinear;
		TextureWrapMode wrapMode = TextureWrapMode::Clamp;

		IntPtr GetNativeTexturePtr();

		const bool IsReadWriteEnable() const
		{
			return enableReadWrite;
		}
	protected:
		bool enableReadWrite;
		ui32 _width = 0;
		ui32 _height = 0;
		void* data = nullptr;
		ui32 pitch;

		TextureDimension _dimension = TextureDimension::None;

		IntPtr _texResource = nullptr;
		IntPtr _texResourceView = nullptr;

		Texture(bool enableReadWrite);
	};
}