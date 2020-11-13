#pragma once

// EXTERNAL INCLUDES
#include <vector>
#include <string>
#if defined(_WINDOWS)
#include <Windows.h>
#elif defined(_LINUX)

#endif
// INTERNAL INCLUDES
#include "math/vec2.h"
#include "math/vec2int.h"
#include "results.h"

namespace DUPLEX_NS_WINDOW
{

	const DUPLEX_NS_MATH::Vec2Int defaultResolution = DUPLEX_NS_MATH::Vec2Int(1280, 720);
#if defined(_WINDOWS)
	const DUPLEX_NS_MATH::Vec2Int defaultPosition = DUPLEX_NS_MATH::Vec2Int(CW_USEDEFAULT, CW_USEDEFAULT);
#elif defined(_LINUX)

#endif
	enum class WindowState
	{
		NORMAL,
		MAXIMIZED,
		MINIMIZED,
		FULLSCREEN
	};

	struct DisplaySettings
	{
		DUPLEX_NS_MATH::Vec2Int resolution;
		ui32 bpp;
		ui32 refreshRate;
		
	};

	struct Display
	{
		DisplaySettings* currentSetting;
		std::vector<DisplaySettings*> settings;

		struct Rect
		{
			int top, left, right, bottom;
		} extends;

		std::string deviceName;
		std::string monitorName;
		std::string displayName;
		bool isPrimary;

#if defined(_WINDOWS)
		HMONITOR monitorHandle;
#endif
		Display()
		{
			currentSetting = nullptr;
			isPrimary = false;
			extends = {};

		}
	};
	
	struct WindowCreateInfo
	{
		const char* windowName;
		DUPLEX_NS_MATH::Vec2Int resolution;
		WindowState currentState;
		DUPLEX_NS_MATH::Vec2Int position;
	};

	class Window
	{
	private:

		friend class WindowManager;

		const char* name;
		DUPLEX_NS_MATH::Vec2Int resolution = defaultResolution;
		Display* currentDisplay;
		DUPLEX_NS_MATH::Vec2Int position;
		DUPLEX_NS_MATH::Vec2Int mousePosition;
		WindowState currentState = WindowState::NORMAL;
#if defined(_WINDOWS)
		HWND windowHandle = NULL;
		HINSTANCE instanceHandle = NULL;
		WNDCLASS windowClass = {};
		DUPLEX_NS_MATH::Vec2Int clientArea;
#elif defined(_LINUX)

#endif
		Window(const WindowCreateInfo* pWindowCreateInfo);

	public:
		bool shouldClose = false;
		DUPLEX_NS_UTIL::DRESULT SetWindowSize(DUPLEX_NS_MATH::Vec2Int newResolution);
		DUPLEX_NS_UTIL::DRESULT SetWindowPosition(DUPLEX_NS_MATH::Vec2Int newPosition);
		DUPLEX_NS_UTIL::DRESULT SetMousePosition(DUPLEX_NS_MATH::Vec2Int newMousePosition);
		DUPLEX_NS_UTIL::DRESULT Minimize();
		DUPLEX_NS_UTIL::DRESULT Maximize();
		DUPLEX_NS_UTIL::DRESULT Restore();
		DUPLEX_NS_UTIL::DRESULT Focus();
	};

	class WindowManager
	{
	public:
		WindowManager();

		~WindowManager();

		void Shutdown();

		DUPLEX_NS_UTIL::DRESULT AddWindow(const WindowCreateInfo* pWindowCreateInfo, Window** window);

		void PollForEvents();

		Display* GetMonitorByHandle(std::string const& displayName);
		Window* GetWindowByHandle(HWND windowHandle);
	private:
		std::vector<Window*> windows;
		std::vector<Display*> displays;
		DUPLEX_NS_MATH::Vec2 mousePosition;

		void InitializeWindow(Window* window);
		void ShutdownWindow(Window* window);
		void GetScreenInfo();

		void CheckWindowScreen(Window* window);
		void ResetDisplays();

#if defined(_WINDOWS)
		MSG winMessage = {};

		static LRESULT CALLBACK WindowProcedure(HWND windowHandle, unsigned int winMessage, WPARAM wordParam, LPARAM longParam);
		
		static BOOL CALLBACK MonitorEnumProcedure(HMONITOR monitorHandle, HDC monitorDeviceContextHandle, LPRECT monitorSize, LPARAM userData);
		
#elif defined(_LINUX)

#endif
	};
}