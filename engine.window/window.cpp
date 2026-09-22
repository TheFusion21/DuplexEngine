#include "window.h"
#include "renderer.h"
#include <SDL.h>

using namespace DUPLEX_NS_WINDOW;
using namespace DUPLEX_NS_GRAPHICS;

bool Window::Init(const char* title, ui32 width, ui32 height, bool vulkanSupport)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			return false;
		}
	}

	Uint32 flags = SDL_WINDOW_RESIZABLE;
	if (vulkanSupport)
	{
		flags |= SDL_WINDOW_VULKAN;
	}

	window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>(width),
		static_cast<int>(height),
		flags
	);
	return window != nullptr;
}

void Window::Shutdown()
{
	if (window)
	{
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	SDL_Quit();
}

void Window::Show()
{
	SDL_ShowWindow(window);
}

void Window::Hide()
{
	SDL_HideWindow(window);
}

void Window::Maximize()
{
	SDL_MaximizeWindow(window);
}

void Window::Minimize()
{
	SDL_MinimizeWindow(window);
}

void Window::SetTitle(const char* title)
{
	SDL_SetWindowTitle(window, title);
}

bool Window::GetClientSize(ui32& width, ui32& height) const
{
	int w = 0, h = 0;
	SDL_GetWindowSize(window, &w, &h);
	width = static_cast<ui32>(w);
	height = static_cast<ui32>(h);
	return true;
}

bool Window::IsFocused() const
{
	return hasFocus;
}

bool Window::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			return false;
		}
		if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(window))
		{
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				return false;
			case SDL_WINDOWEVENT_RESIZED:
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				if (renderer)
				{
					ui32 width, height;
					GetClientSize(width, height);
					renderer->Resize(width, height);
				}
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				hasFocus = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				hasFocus = false;
				break;
			}
		}
	}
	return true;
}

void Window::SetRenderer(Renderer* renderer)
{
	this->renderer = renderer;
}

SDL_Window* Window::GetSDLWindow() const
{
	return window;
}
