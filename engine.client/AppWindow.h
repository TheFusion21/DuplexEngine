#pragma once

// EXTERNAL INCLUDES
#include <Windows.h>
// INTERNAL INCLUDES
#include "math/types.h"

namespace Engine::Core
{
	class AppWindow
	{
	private:
		HWND hwnd = NULL;
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
		ui64 GetHandle();
	};
}
