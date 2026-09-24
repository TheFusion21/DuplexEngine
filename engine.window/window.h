#pragma once
// EXTERNAL INCLUDES
#include <functional>
// INTERNAL INCLUDES
#include "math/types.h"
#include "namespaces.h"

struct SDL_Window;
union SDL_Event;

namespace DUPLEX_NS_GRAPHICS
{
	class Renderer;
}

namespace DUPLEX_NS_WINDOW
{
	// Borderless: the window is resized to cover the whole monitor at the desktop's current
	// resolution (SDL_WINDOW_FULLSCREEN_DESKTOP) - the OS display mode itself never changes, so
	// the desktop compositor keeps running and alt-tab is instant with no flicker. Exclusive: a
	// real display-mode change (SDL_WINDOW_FULLSCREEN) - SDL picks the closest mode to the
	// window's current size (no resolution picker exists yet - see docs/roadmap Phase 15/16 for
	// where a real one would live). Deliberately implemented once here, at the SDL/window layer,
	// rather than per-backend (e.g. D3D11/D3D12's own DXGI SetFullscreenState): SDL's resize
	// event already drives Renderer::Resize() on every backend identically (see PollEvents()
	// below), and Vulkan has no DXGI-exclusive-fullscreen equivalent at all without the
	// Windows-only, opt-in VK_EXT_full_screen_exclusive extension - this way all three backends
	// get both modes uniformly instead of D3D11/D3D12 getting a "truer" exclusive fullscreen
	// Vulkan structurally can't match.
	enum class FullscreenMode
	{
		Windowed,
		Borderless,
		Exclusive
	};

	// Thin SDL2 wrapper for a single window. Both engine.client and engine.editor only ever
	// need one window at a time, so there's no manager layer here - just this.
	class Window
	{
	private:
		SDL_Window* window = nullptr;
		bool hasFocus = false;
		FullscreenMode fullscreenMode = FullscreenMode::Windowed;
		// Non-owning. Set once the Renderer exists (which needs the window first), so it can't
		// just happen in Init(). Used by PollEvents() to forward resize events - SDL's own
		// event queue replaces the WM_SIZE/WinProc callback pattern the old Win32 code used.
		DUPLEX_NS_GRAPHICS::Renderer* renderer = nullptr;

	public:
		// vulkanSupport must be true if this window will be passed to VulkanRenderer::Init -
		// SDL_CreateWindow outright fails if asked for Vulkan support on a video driver/platform
		// that doesn't have it (verified: SDL's headless "dummy" driver is one such case), so
		// this isn't requested unless it's actually needed.
		bool Init(const char* title, ui32 width, ui32 height, bool vulkanSupport = false);
		void Shutdown();

		void Show();
		void Hide();
		void Maximize();
		void Minimize();

		void SetTitle(const char* title);

		bool GetClientSize(ui32& width, ui32& height) const;
		bool IsFocused() const;

		// Switches between windowed/borderless/exclusive fullscreen (see FullscreenMode above).
		// Resizes the backbuffer through the already-set Renderer (see SetRenderer) before
		// returning, rather than waiting for the resize event PollEvents() would otherwise pick
		// up on the next pump - a caller toggling this mid-frame (e.g. an F11 keybind) expects
		// the new size to be in effect immediately. Returns false if SDL itself rejected the
		// mode switch (fullscreenMode/GetFullscreenMode() are left unchanged in that case).
		bool SetFullscreenMode(FullscreenMode mode);
		FullscreenMode GetFullscreenMode() const;

		// Pumps the SDL event queue. Handles resize internally (see renderer above). Returns
		// false once the window should close (SDL_QUIT, or the window's close button).
		// onEvent, if given, is invoked with every raw event as it's pumped (before this
		// class's own handling) - e.g. so engine.editor can forward each one to
		// ImGui_ImplSDL2_ProcessEvent without this class needing to know ImGui exists.
		bool PollEvents(const std::function<void(const SDL_Event&)>& onEvent = nullptr);

		void SetRenderer(DUPLEX_NS_GRAPHICS::Renderer* renderer);

		// For backends that need the real SDL handle: Vulkan needs this for
		// SDL_Vulkan_CreateSurface, D3D11 needs it for SDL_GetWindowWMInfo. See Renderer::Init.
		SDL_Window* GetSDLWindow() const;
	};
}
