#pragma once

namespace Engine::Graphics
{
	enum class BufferType
	{
		Index,
		Vertex,
		Constant,
		DepthStencil,
		ShaderResource,
	};
	enum class UsageType
	{
		Default,
		Dynamic
	};
	enum class TextureFormat
	{
		//RGB color and alpha with 32-bit floats each.
		RGBAFLOAT,
		//RGB color with 32-bit floats each.
		RGBFLOAT,
		//RGB color and alpha with 8-bits unsigned-normalized-integer each.
		RGBA32,
		//alpa only with 8-bits unsigned-normalized-integer
		ALPHA8,
		//red only with 8-bits unsigned-normalized-integer
		RED8,
		REDFLOAT,
		//red only with 1-bits unsigned-normalized-integer
		RED1,
		//depth
		D32
	};
}