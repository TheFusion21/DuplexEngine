#include "texture.h"

using namespace DUPLEX_NS_GRAPHICS;

unsigned long Texture::currentTextureMemory = 0;

ShaderResourceViewHandle Texture::GetShaderResourceView()
{
	return _texResourceView;
}

Texture::Texture(bool enableReadWrite) : enableReadWrite(enableReadWrite)
{

}