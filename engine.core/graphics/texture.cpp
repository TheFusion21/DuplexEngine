#include "texture.h"
#include "texture.h"

using namespace Engine::Graphics;

unsigned long Texture::currentTextureMemory = 0;

IntPtr Engine::Graphics::Texture::GetNativeTexturePtr()
{
	return nullptr;
}

Texture::Texture(bool enableReadWrite) : dimension(_dimension), width(_width), height(_height), enableReadWrite(enableReadWrite)
{

}