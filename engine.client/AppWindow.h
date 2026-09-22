#pragma once

// EXTERNAL INCLUDES
#define NOMINMAX
#include <Windows.h>
// INTERNAL INCLUDES
#include "math/types.h"
#include "namespaces.h"

namespace DUPLEX_NS_GRAPHICS
{
	class Renderer;
}

namespace Engine::Core
{
	class AppWindow
	{
	private:
		HWND hwnd = NULL;
		HINSTANCE instance;
		bool hasFocus = false;
		// Non-owning: set once Application has created the Renderer (Resize/fullscreen
		// handling happens from WinProc, which has no other way to reach it now that Renderer
		// isn't a global singleton).
		DUPLEX_NS_GRAPHICS::Renderer* renderer = nullptr;
	public:
		bool isInit = false;
		void Init(AnsiString name, int width, int height);
		void Show();
		void Hide();
		void Maximize();
		void Minimize();
		bool Resize();
		void ResizeOnFullscreenToggle();
		bool MessagePump();
		bool GetClientSize(ui32&width, ui32& height);
		bool IsFocused();
		void SetFocus(bool focus);
		ui64 GetHandle();
		ui64 GetInstance();
		void SetRenderer(DUPLEX_NS_GRAPHICS::Renderer* renderer);

		void SetTitle(const char* title);
		const char* GetTitle();
	};
}
