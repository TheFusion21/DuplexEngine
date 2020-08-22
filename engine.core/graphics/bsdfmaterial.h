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
		Texture2D albedo = Texture2D::FromColor(2, 2, false, { 1.0f, 1.0f, 1.0f, 1.0f });
		/// metallic description of material (uses only Red channel)
		Texture2D metallic = Texture2D::FromValue(2, 2, false, 0.0f, Texture2D::SingleChannelMode::RED);
		/// AO description of material (uses only Red channel)
		Texture2D ambientOcclusion = Texture2D::FromColor(2, 2, false, { 1.0f, 1.0f, 1.0f, 1.0f });
		/// roughness description of material (uses only Red channel)
		Texture2D roughness = Texture2D::FromValue(2, 2, false, 0.5f, Texture2D::SingleChannelMode::RED);

		Texture2D normal = Texture2D::FromColor(2, 2, false, { 0.5f, 0.5f, 1.0f, 1.0f });

		Texture2D emission = Texture2D::FromColor(2, 2, false, { 0.0f, 0.0f, 0.0f, 1.0f });
	};
}