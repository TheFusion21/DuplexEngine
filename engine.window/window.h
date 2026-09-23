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
	// Thin SDL2 wrapper for a single window. Both engine.client and engine.editor only ever
	// need one window at a time, so there's no manager layer here - just this.
	class Window
	{
	private:
		SDL_Window* window = nullptr;
		bool hasFocus = false;
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
