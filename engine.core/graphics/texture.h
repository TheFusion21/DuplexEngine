#pragma once
#include "math/types.h"
namespace Engine::Graphics
{
	enum class TextureFormat
	{
		//RGB color and alpha with 32-bit floats each.
		RGBAFLOAT,
		//RGB color and alpha with 8-bits unsigned-normalized-integer each.
		RGBA32,
		//alpa only with 8-bits unsigned-normalized-integer
		ALPHA8,
		//red only with 8-bits unsigned-normalized-integer
		RED8,
		//red only with 1-bits unsigned-normalized-integer
		RED1
	};

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

	enum class SingleChannelMode
	{
		RED,
		ALPHA
	};
	class Texture
	{
	public:
		static unsigned long currentTextureMemory;

		const ui32& width; ///< Pixel width of the texture (Read Only)
		const ui32& height; ///< Pixel height of the texture (Read Only)
		const TextureDimension& dimension;

		FilterMode filterMode = FilterMode::Bilinear;
		TextureWrapMode wrapMode = TextureWrapMode::Clamp;

		IntPtr GetNativeTexturePtr();

		const bool enableReadWrite;
	protected:

		ui32 _width = 0;
		ui32 _height = 0;
		void* data = nullptr;
		ui32 pitch;

		TextureDimension _dimension = TextureDimension::None;

		IntPtr _texResource = nullptr;

		Texture(bool enableReadWrite);
	};
}