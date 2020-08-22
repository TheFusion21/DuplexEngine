#include "texture.h"
#include "texture.h"

using namespace Engine::Graphics;

unsigned long Texture::currentTextureMemory = 0;

IntPtr Engine::Graphics::Texture::GetNativeTexturePtr()
{
	return _texResource;
}

Texture::Texture(bool enableReadWrite) : enableReadWrite(enableReadWrite)
{

}