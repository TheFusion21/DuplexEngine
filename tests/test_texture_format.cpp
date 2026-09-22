// Regression test for Renderer::PixelSizeFromTextureFormat (driver.graphics/renderer.cpp).
// This is real production logic shared by every backend (it decides how many bytes to
// allocate per texel), so it's worth pinning down now, before any backend touches it.
//
// Renderer is abstract, so a minimal no-op subclass is used purely to get access to the
// (protected) function under test - none of the overrides below are exercised.

#include "renderer.h"
#include <cstdio>

using namespace DUPLEX_NS_GRAPHICS;

namespace
{
	class NullRenderer : public Renderer
	{
	public:
		bool Init(ui64, ui64, ui32, ui32) override { return true; }
		void SetViewPort() override {}
		void CreateShader() override {}
		void SetActiveCamera(DUPLEX_NS_MATH::Vec3, DUPLEX_NS_MATH::Mat4x4) override {}
		void ClearLights() override {}
		void SetLight(DUPLEX_NS_UTIL::GpuLight) override {}
		void BeginScene() override {}
		void EndScene() override {}
		void Render(DUPLEX_NS_MATH::Mat4x4, BufferHandle, BufferHandle, ui32) override {}
		void Shutdown() override {}
		TextureHandle CreateTexture(ui32, ui32, ui32, TextureFormat, void* = nullptr) override { return TextureHandle{}; }
		ShaderResourceViewHandle CreateTextureSRV(TextureHandle, TextureFormat) override { return ShaderResourceViewHandle{}; }
		void UseTexture(ui32, ShaderResourceViewHandle) override {}
		ShaderResourceViewHandle CreateCubemapSRV(TextureHandle, TextureFormat) override { return ShaderResourceViewHandle{}; }
		BufferHandle CreateBuffer(BufferType, const void*, int, UsageType = UsageType::Default) override { return BufferHandle{}; }
		bool Resize(ui32, ui32) override { return true; }
		bool CheckForFullscreen() override { return false; }
		void ReleaseTexture(TextureHandle&) override {}
		void ReleaseTextureSRV(ShaderResourceViewHandle&) override {}
		void ReleaseBuffer(BufferHandle&) override {}

		ui32 PublicPixelSize(TextureFormat format) { return PixelSizeFromTextureFormat(format); }
	};

	int failures = 0;

	void CheckEq(ui32 actual, ui32 expected, const char* label)
	{
		if (actual != expected)
		{
			std::fprintf(stderr, "FAIL: %s -> expected %u, got %u\n", label, expected, actual);
			failures++;
		}
	}
}

int main()
{
	NullRenderer renderer;

	CheckEq(renderer.PublicPixelSize(TextureFormat::RGBAFLOAT), 16, "RGBAFLOAT");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RGBFLOAT), 12, "RGBFLOAT");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RGBA32), 4, "RGBA32");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RGBA64), 8, "RGBA64");
	CheckEq(renderer.PublicPixelSize(TextureFormat::ALPHA8), 1, "ALPHA8");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RED8), 1, "RED8");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RED16), 2, "RED16");
	CheckEq(renderer.PublicPixelSize(TextureFormat::REDFLOAT), 4, "REDFLOAT");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RGFLOAT), 8, "RGFLOAT");
	CheckEq(renderer.PublicPixelSize(TextureFormat::RG32), 4, "RG32");
	// NOTE: D32 currently returns sizeof(byte) == 1 in renderer.cpp. For a 32-bit depth
	// format that looks like a pre-existing bug (probably should be 4), but fixing rendering
	// correctness is out of scope for this pass - pinned to current behavior so a real fix
	// shows up here as an intentional test change, not a silent regression.
	CheckEq(renderer.PublicPixelSize(TextureFormat::D32), 1, "D32 (suspected pre-existing bug, see comment)");

	if (failures == 0)
	{
		std::printf("test_texture_format: all checks passed\n");
		return 0;
	}
	std::fprintf(stderr, "test_texture_format: %d check(s) failed\n", failures);
	return 1;
}
