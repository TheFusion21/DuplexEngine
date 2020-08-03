#pragma once
#include "utils/color.h"
#include "texture.h"

namespace Engine::Graphics
{
	class Texture2D : public Texture
	{
	public:
		
		static Texture2D blackTexture; ///< Get a black texture of 1 pixel size
		static Texture2D whiteTexture; ///< Get a white texture of 1 pixel size
		static Texture2D grayTexture; ///< Get a gray texture of 1 pixel size
		static Texture2D normalTexture; ///< Get a normal texture of 1 pixel size

		/// Create a uniform Texture2D with specific width and height preferably power of 2
		static Texture2D FromColor(ui32 width, ui32 height, bool enableReadWrite, Engine::Utils::FloatColor color);
		static Texture2D FromValue(ui32 width, ui32 height, bool enableReadWrite, float value, SingleChannelMode mode = SingleChannelMode::RED);
		
		static Texture2D LoadFromFile(const char* filename, bool enableReadWrite);


		const TextureFormat& format; ///< The format pixel data is stored in (Read Only)
		

		/// Create a Texture2D with specific width and height preferably power of 2
		Texture2D(ui32 width, ui32 height, bool enableReadWrite);

		/// Create a Texture2D with specific width and height preferably power of 2 and format the data is stored in
		Texture2D(ui32 width, ui32 height, bool enableReadWrite, TextureFormat format);

		

		/// resizes a Texture2D to new dimensions
		void Resize(ui32 width, ui32 height);
	private:
		TextureFormat _format = TextureFormat::RGBA32; ///< The format pixel data is stored in

		

		
	}; // end of class Texture2D
}