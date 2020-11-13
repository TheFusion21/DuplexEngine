#include "window.h"

using namespace DUPLEX_NS_WINDOW;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;


Window::Window(const WindowCreateInfo* pWindowCreateInfo)
{
	this->name = pWindowCreateInfo->windowName;
	this->resolution = pWindowCreateInfo->resolution;
	this->position = pWindowCreateInfo->position;
}
#if defined(_WINDOWS)
DRESULT Window::SetWindowSize(Vec2Int newResolution)
{
	this->resolution = newResolution;

	return DRESULT::OK;
}

DRESULT Window::SetWindowPosition(Vec2Int newPosition)
{
	this->position = newPosition;

	return DRESULT::OK;
}
DRESULT Window::SetMousePosition(Math::Vec2Int newMousePosition)
{
	return DRESULT::OK;
}
DRESULT Window::Minimize()
{
	return DRESULT();
}
DRESULT Window::Maximize()
{
	return DRESULT();
}
DRESULT Window::Restore()
{
	return DRESULT();
}
DRESULT Window::Focus()
{
	return DRESULT();
}
#elif defined(_LINUX)

#endif


WindowManager::WindowManager()
{
#if defined(_WINDOWS)
	HWND desktopHandle = GetDesktopWindow();

	if (desktopHandle)
	{
		GetScreenInfo();

	}
#endif 

}

WindowManager::~WindowManager()
{
	Shutdown();
}

void WindowManager::Shutdown()
{
	ResetDisplays();
	for (auto window : windows)
	{
		ShutdownWindow(window);
	}
	windows.clear();
}


DRESULT WindowManager::AddWindow(const WindowCreateInfo* pWindowCreateInfo, Window** window)
{
	if (pWindowCreateInfo != nullptr)
	{
		WindowCreateInfo wdi = {};
		if (pWindowCreateInfo->resolution == Vec2Int(0, 0))
			wdi.resolution = defaultResolution;
		else
			wdi.resolution = pWindowCreateInfo->resolution;

		Window* newWindow = new Window(pWindowCreateInfo);
		windows.push_back(newWindow);
		InitializeWindow(windows.back());

		*window = newWindow;
		return DRESULT::OK;
	}
	
	return DRESULT::INVALIDCREATEINFO;
}

#if defined(_WINDOWS)
void WindowManager::PollForEvents()
{
	while (PeekMessage(&winMessage, nullptr, 0, 0, PM_REMOVE))
	{
		//the only place I can see this being needed if someone called PostQuitMessage manually
		TranslateMessage(&winMessage);
		DispatchMessage(&winMessage);
		if (winMessage.message == WM_QUIT)
		{
			Shutdown();
		}
	}
}
#elif defined(_LINUX)
inline void WindowManager::PollForEvents()
{

}
#endif
Display* WindowManager::GetMonitorByHandle(std::string const& displayName)
{
	for (auto& display : displays)
	{
		if (displayName.compare(display->displayName) == 0)
		{
			return display;
		}
	}
	return nullptr;
}
Window* WindowManager::GetWindowByHandle(HWND windowHandle)
{
	for (auto& window : windows)
	{
		if (window->windowHandle == windowHandle)
		{
			return window;
		}
	}
	return nullptr;
}

#if defined(_WINDOWS)
LRESULT WindowManager::WindowProcedure(HWND windowHandle, unsigned int winMessage, WPARAM wordParam, LPARAM longParam)
{
	WindowManager* manager = reinterpret_cast<WindowManager*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

	Window* window = nullptr;
	if (manager)
	{
		window = manager->GetWindowByHandle(windowHandle);
	}

	switch (winMessage)
	{
		case WM_DESTROY:
		{
			if (manager && window)
			{
				window->shouldClose = true;

			}
			break;
		}
		case WM_MOVE:
		{
			if (window && manager)
			{
				window->position.x = LOWORD(longParam);
				window->position.y = HIWORD(longParam);
				manager->CheckWindowScreen(window);
			}
			break;
		}
		case WM_MOVING:
		{
			if (window && manager)
			{
				window->position.x = LOWORD(longParam);
				window->position.y = HIWORD(longParam);
			}
			break;
		}
		case WM_SIZE:
		{
			if (window && manager)
			{
				window->clientArea.x = (ui32)LOWORD(longParam);
				window->clientArea.y = (ui32)HIWORD(longParam);

				switch (wordParam)
				{
					case SIZE_MAXIMIZED:
					{
						break;
					}
					case SIZE_MINIMIZED:
					{
						break;
					}
					default:
					{
						break;
					}
				}
			}
			break;
		}
		case WM_SIZING:
		{
			if (window && manager)
			{
				window->resolution.x = (unsigned int)LOWORD(longParam);
				window->resolution.y = (unsigned int)HIWORD(longParam);
			}
			break;
		}
		case WM_INPUT:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_CHAR:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_KEYDOWN:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_KEYUP:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_SYSKEYDOWN:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_SYSKEYUP:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_LBUTTONUP:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_RBUTTONDOWN:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_RBUTTONUP:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_MBUTTONDOWN:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_MBUTTONUP:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_MOUSEWHEEL:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_SETFOCUS:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_KILLFOCUS:
		{
			if (window && manager)
			{

			}
			break;
		}
		case WM_DROPFILES:
		{
			if (window && manager)
			{

			}
			break;
		}
		default:
		{
			return DefWindowProc(windowHandle, winMessage, wordParam, longParam);
		}
	}
	return 0;
}
void WindowManager::InitializeWindow(Window* window)
{
	window->instanceHandle = GetModuleHandle(nullptr);
	window->windowClass = {};
	window->windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
	window->windowClass.lpfnWndProc = WindowManager::WindowProcedure;
	window->windowClass.hInstance = window->instanceHandle;
	window->windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	window->windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	window->windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	window->windowClass.lpszMenuName = window->name;
	window->windowClass.lpszClassName = window->name;
	RegisterClass(&window->windowClass);

	window->windowHandle = CreateWindow(window->name, window->name, WS_OVERLAPPEDWINDOW
		,window->position.x, window->position.y
		,window->resolution.x, window->resolution.y
		,nullptr, nullptr, nullptr, nullptr);

	SetWindowLongPtr(window->windowHandle, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(window->windowHandle, SW_SHOWNORMAL);
	UpdateWindow(window->windowHandle);

	
	CheckWindowScreen(window);
	/*
	window->SetStyle(style_t::normal);
	*/
	DragAcceptFiles(window->windowHandle, true);

}
void WindowManager::ShutdownWindow(Window* window)
{
	window->shouldClose = true;
	UnregisterClass(window->name, window->instanceHandle);
	FreeModule(window->instanceHandle);

	window->windowHandle = nullptr;

	for (auto it = windows.begin(); it != windows.end(); ++it)
	{
		if (window == *it)
		{
			windows.erase(it);
			break;
		}
	}
}
void WindowManager::GetScreenInfo()
{
	DISPLAY_DEVICE graphicsDevice;
	graphicsDevice.cb = sizeof(DISPLAY_DEVICE);
	graphicsDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
	DWORD deviceNum = 0;
	DWORD monitorNum = 0;
	while (EnumDisplayDevices(nullptr, deviceNum, &graphicsDevice, EDD_GET_DEVICE_INTERFACE_NAME))
	{
		DISPLAY_DEVICE monitorDevice = { 0 };
		monitorDevice.cb = sizeof(DISPLAY_DEVICE);
		monitorDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

		Display* display = nullptr;

		while (EnumDisplayDevices(graphicsDevice.DeviceName, monitorNum, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME))
		{
			display = new Display();
			display->displayName = graphicsDevice.DeviceName;
			display->deviceName = graphicsDevice.DeviceString;
			display->monitorName = graphicsDevice.DeviceString;
			display->isPrimary = (graphicsDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) == DISPLAY_DEVICE_PRIMARY_DEVICE;

			DEVMODE devmode = {};
			unsigned int modeIndex = UINT_MAX;
			while (EnumDisplaySettings(graphicsDevice.DeviceName, modeIndex, &devmode))
			{
				if (modeIndex == ENUM_CURRENT_SETTINGS)
				{
					display->currentSetting = new DisplaySettings();
					display->currentSetting->resolution.x = devmode.dmPelsWidth;
					display->currentSetting->resolution.y = devmode.dmPelsHeight;
					display->currentSetting->bpp = devmode.dmBitsPerPel;
					display->currentSetting->refreshRate = devmode.dmDisplayFrequency;
				}
				else
				{
					DisplaySettings* newSettings = new DisplaySettings();
					newSettings->resolution.x = devmode.dmPelsWidth;
					newSettings->resolution.y = devmode.dmPelsHeight;
					newSettings->bpp = devmode.dmBitsPerPel;
					newSettings->refreshRate = devmode.dmDisplayFrequency;
					display->settings.push_back(newSettings);
				}
				modeIndex++;
			}
			displays.push_back(display);
			monitorNum++;
			monitorDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
		}
		deviceNum++;
		monitorNum = 0;
	}
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcedure, (LPARAM)this);
}
BOOL WindowManager::MonitorEnumProcedure(HMONITOR monitorHandle, HDC monitorDeviceContextHandle, LPRECT monitorSize, LPARAM userData)
{
	WindowManager* manager = reinterpret_cast<WindowManager*>(userData);

	MONITORINFOEX info = {};
	info.cbSize = sizeof(info);
	GetMonitorInfo(monitorHandle, &info);

	Display* display = manager->GetMonitorByHandle(info.szDevice);
	display->monitorHandle = monitorHandle;
	display->extends = { monitorSize->top, monitorSize->left, monitorSize->right, monitorSize->bottom };
	return true;
}

void WindowManager::CheckWindowScreen(Window* window)
{
	HMONITOR currentHandle = MonitorFromWindow(window->windowHandle, MONITOR_DEFAULTTONEAREST);
	for (auto& display : displays)
	{
		if (display->monitorHandle == currentHandle)
		{
			window->currentDisplay = display;
		}
	}

}
void WindowManager::ResetDisplays()
{
	for (auto display : displays)
	{
		ChangeDisplaySettingsEx(display->displayName.c_str(), nullptr, nullptr, CDS_FULLSCREEN, nullptr);
	}
}
#elif defined(_LINUX)


#endif


