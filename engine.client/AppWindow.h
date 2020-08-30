#pragma once

// EXTERNAL INCLUDES
#define NOMINMAX
#include <Windows.h>
// INTERNAL INCLUDES
#include "math/types.h"

namespace Engine::Core
{
	class AppWindow
	{
	private:
		HWND hwnd = NULL;
		HINSTANCE instance;
		bool hasFocus = false;
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

		void SetTitle(const char* title);
		const char* GetTitle();
	};
}
