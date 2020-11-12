// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "appwindow.h"
#include "appinfo.h"
#include "d3d11renderer.h"

using namespace DUPLEX_NS_GRAPHICS;
using namespace Engine::Core;

LRESULT WINAPI WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AppWindow* app = reinterpret_cast<AppWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	switch (uMsg)
	{
	case WM_CREATE:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>((((CREATESTRUCT*)lParam)->lpCreateParams)));
		break;
	case WM_QUIT:
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		break;
	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_SIZE:
		if(app->isInit)app->Resize();
		break;
	case WM_WINDOWPOSCHANGED:
		if (app->isInit)app->ResizeOnFullscreenToggle();
		break;
	case WM_SETFOCUS:
		app->SetFocus(true);
		break;
	case WM_KILLFOCUS:
		app->SetFocus(false);
			break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
			app->SetFocus(false);
		else
			app->SetFocus(true);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void AppWindow::Init(AnsiString name, int width, int height)
{
	AnsiString className = "WinClass";

	WNDCLASSEX wc = {};
	//Let windows know which wnd class type we use
	wc.cbSize = sizeof(wc);
	instance = wc.hInstance = GetModuleHandleA(NULL);
	//Enable redraw for vertical and horizontal size change
	wc.style = CS_HREDRAW | CS_VREDRAW;
	//Set the icon for the application
	wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_APPICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//Assign process to this class
	wc.lpfnWndProc = WinProc;
	//Label the class for identification for window
	wc.lpszClassName = className;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	//Register Class for preperation to create window
	if (FAILED(RegisterClassEx(&wc)))
	{
		MessageBoxA(NULL, "HAT NICHT FUNZT", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	}
	//Create Window with specified styles and assigned window class
	this->hwnd = CreateWindowEx(
		0,
		className,
		name,
		WS_OVERLAPPEDWINDOW | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		wc.hInstance,
		this
	);
	//Message Client if window could not 
	if (this->hwnd == NULL)
	{
		MessageBoxA(NULL, "HAT NICHT FUNZT", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	}
	//Display the Window
	this->Show();
	isInit = true;
}

void AppWindow::Show()
{
	ShowWindow(hwnd, SW_SHOW);
}

void AppWindow::Hide()
{
	ShowWindow(hwnd, SW_HIDE);
}

void AppWindow::Maximize()
{
	ShowWindow(hwnd, SW_MAXIMIZE);
}

void AppWindow::Minimize()
{
	ShowWindow(hwnd, SW_MINIMIZE);
}

bool AppWindow::Resize()
{
	ui32 width, height;
	GetClientSize(width, height);
	return Renderer::GetInstancePtr()->Resize(width, height);
}

void Engine::Core::AppWindow::ResizeOnFullscreenToggle()
{
	if (Renderer::GetInstancePtr()->CheckForFullscreen())
	{
		Resize();
	}
}

bool AppWindow::MessagePump()
{
	//Read Message and Dispatch them for WinProc
	MSG msg = { };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

bool Engine::Core::AppWindow::GetClientSize(ui32& width, ui32& height)
{
	RECT rect = {};
	if (GetClientRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		return true;
	}
	return false;
}

bool Engine::Core::AppWindow::IsFocused()
{
	return hasFocus;
}

void Engine::Core::AppWindow::SetFocus(bool focus)
{
	hasFocus = focus;
}

ui64 Engine::Core::AppWindow::GetHandle()
{
	return reinterpret_cast<ui64>(hwnd);
}

ui64 Engine::Core::AppWindow::GetInstance()
{
	return reinterpret_cast<ui64>(instance);
}

void Engine::Core::AppWindow::SetTitle(const char* title)
{
	if (!isInit)
		return;
	SetWindowTextA(hwnd, title);
}

const char* Engine::Core::AppWindow::GetTitle()
{
	if (!isInit)
		return nullptr;

	int maxCount = 256;
	char* title = new char[maxCount];
	GetWindowTextA(hwnd, title, maxCount);
	return title;
}
