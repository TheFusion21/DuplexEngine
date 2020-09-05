#include "texture.h"

using namespace Engine::Graphics;

unsigned long Texture::currentTextureMemory = 0;

IntPtr Texture::GetNativeTexturePtr()
{
	return _texResourceView;
}

Texture::Texture(bool enableReadWrite) : enableReadWrite(enableReadWrite)
{

}