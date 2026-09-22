#pragma once
#include "texture2d.h"

namespace DUPLEX_NS_GRAPHICS
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
		Texture2D albedo;
		/// metallic description of material (uses only Red channel)
		Texture2D metallic;
		/// AO description of material (uses only Red channel)
		Texture2D ambientOcclusion;
		/// roughness description of material (uses only Red channel)
		Texture2D roughness;

		Texture2D normal;

		Texture2D emission;

		// Texture2D has no default constructor (it always needs a Renderer to create its GPU
		// resource), so the defaults that used to live as default member initializers now live
		// here instead, where a Renderer is actually available.
		explicit BsdfMaterial(Renderer& renderer) :
			albedo(Texture2D::FromColor(renderer, 2, 2, false, { 1.0f, 1.0f, 1.0f, 1.0f })),
			metallic(Texture2D::FromValue(renderer, 2, 2, false, 0.0f, Texture2D::SingleChannelMode::RED)),
			ambientOcclusion(Texture2D::FromColor(renderer, 2, 2, false, { 1.0f, 1.0f, 1.0f, 1.0f })),
			roughness(Texture2D::FromValue(renderer, 2, 2, false, 0.5f, Texture2D::SingleChannelMode::RED)),
			normal(Texture2D::FromColor(renderer, 2, 2, false, { 0.5f, 0.5f, 1.0f, 1.0f })),
			emission(Texture2D::FromColor(renderer, 2, 2, false, { 0.0f, 0.0f, 0.0f, 1.0f }))
		{
		}
	};
}
