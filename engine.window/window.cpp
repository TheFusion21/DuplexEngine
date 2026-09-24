#include "window.h"
#include "renderer.h"
#include "log.h"
#include <SDL.h>

using namespace DUPLEX_NS_WINDOW;
using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_LOG;

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

bool Window::SetFullscreenMode(FullscreenMode mode)
{
	Uint32 flag = 0;
	switch (mode)
	{
	case FullscreenMode::Borderless:
		flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
	case FullscreenMode::Exclusive:
		flag = SDL_WINDOW_FULLSCREEN;
		break;
	case FullscreenMode::Windowed:
	default:
		flag = 0;
		break;
	}

	if (SDL_SetWindowFullscreen(window, flag) != 0)
	{
		Logger::Window().error("SDL_SetWindowFullscreen failed: {}", SDL_GetError());
		return false;
	}
	fullscreenMode = mode;

	if (renderer)
	{
		ui32 width, height;
		GetClientSize(width, height);
		renderer->Resize(width, height);
		Logger::Window().info("Fullscreen mode changed to {} ({}x{})",
			mode == FullscreenMode::Windowed ? "Windowed" : mode == FullscreenMode::Borderless ? "Borderless" : "Exclusive",
			width, height);
	}
	return true;
}

FullscreenMode Window::GetFullscreenMode() const
{
	return fullscreenMode;
}

bool Window::PollEvents(const std::function<void(const SDL_Event&)>& onEvent)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (onEvent)
			onEvent(event);

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
