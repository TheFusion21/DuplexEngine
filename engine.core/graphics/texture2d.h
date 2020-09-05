#pragma once
#include "utils/color.h"
#include "texture.h"

namespace Engine::Graphics
{
	class Texture2D : public Texture
	{
	public:
		enum class SingleChannelMode
		{
			RED,
			ALPHA
		};
		/// Create a uniform Texture2D with specific width and height preferably power of 2
		static Texture2D FromColor(ui32 width, ui32 height, bool enableReadWrite, Engine::Utils::FloatColor color);
		static Texture2D FromValue(ui32 width, ui32 height, bool enableReadWrite, float value, SingleChannelMode mode = SingleChannelMode::RED);
		
		static Texture2D LoadFromFile(const char* filename, bool enableReadWrite);

		~Texture2D();
		/// Create a Texture2D with specific width and height preferably power of 2
		Texture2D(ui32 width, ui32 height, bool enableReadWrite);

		/// Create a Texture2D with specific width and height preferably power of 2 and format the data is stored in
		Texture2D(ui32 width, ui32 height, bool enableReadWrite, TextureFormat format);

		
		/// resizes a Texture2D to new dimensions
		void Resize(ui32 width, ui32 height);
	};
}