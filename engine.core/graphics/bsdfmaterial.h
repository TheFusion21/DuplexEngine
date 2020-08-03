#pragma once
#include "texture2d.h"

namespace Engine::Graphics
{
	enum class SurfaceType
	{
		Opaque,
		Transparent
	};
	class BsdfMaterial
	{
	public:
		SurfaceType surfaceType = SurfaceType::Opaque;
		/// base color of the material
		Texture2D albedo = Texture2D::whiteTexture;
		/// metallic description of material (uses only Red channel)
		Texture2D metallic = Texture2D::blackTexture;
		/// AO description of material (uses only Red channel)
		Texture2D ambientOcclusion = Texture2D::whiteTexture;
		/// roughness description of material (uses only Red channel)
		Texture2D roughness = Texture2D::blackTexture;

		Texture2D normal = Texture2D::normalTexture;

		Texture2D emission = Texture2D::blackTexture;
	};
}