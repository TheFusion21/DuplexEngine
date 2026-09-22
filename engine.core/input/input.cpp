#include "input.h"
#include "utils/util.h"
#include <SDL.h>


using namespace DUPLEX_NS_UTIL;
using namespace DUPLEX_NS_MATH;

Input::KeyState* Input::keyStates = nullptr;
ui32 Input::keyStateCount = 0;

Vec2 Input::_mousePosition = Vec2Zero;
const Vec2& Input::mousePosition = Input::_mousePosition;

void Input::Init()
{
	int count = 0;
	SDL_GetKeyboardState(&count);
	keyStateCount = static_cast<ui32>(count);
	keyStates = new KeyState[keyStateCount];
	for (ui32 i = 0; i < keyStateCount; i++)
	{
		keyStates[i] = KeyState::NONE;
	}

	int x = 0, y = 0;
	SDL_GetMouseState(&x, &y);
	_mousePosition.x = static_cast<real>(x);
	_mousePosition.y = static_cast<real>(y);
}

void Input::Update()
{
	int x = 0, y = 0;
	SDL_GetMouseState(&x, &y);
	_mousePosition.x = static_cast<real>(x);
	_mousePosition.y = static_cast<real>(y);

	for (ui32 i = 0; i < keyStateCount; i++)
	{
		if (keyStates[i] == KeyState::PRESSED)
			keyStates[i] = KeyState::DOWN;
		else if (keyStates[i] == KeyState::RELEASED)
			keyStates[i] = KeyState::NONE;
	}

	const Uint8* sdlKeyStates = SDL_GetKeyboardState(nullptr);
	for (ui32 i = 0; i < keyStateCount; i++)
	{
		if (sdlKeyStates[i])
		{
			if (keyStates[i] == KeyState::NONE || keyStates[i] == KeyState::RELEASED)
			{
				keyStates[i] = KeyState::PRESSED;
			}
		}
		else if (keyStates[i] == KeyState::DOWN || keyStates[i] == KeyState::PRESSED)
		{
			keyStates[i] = KeyState::RELEASED;
		}
	}
}

bool Input::GetKey(int keycode)
{
	if (keycode < 0 || static_cast<ui32>(keycode) >= keyStateCount)
		return false;
	return (keyStates[keycode] == KeyState::PRESSED || keyStates[keycode] == KeyState::DOWN);
}

bool Input::GetKeyDown(int keycode)
{
	if (keycode < 0 || static_cast<ui32>(keycode) >= keyStateCount)
		return false;
	return keyStates[keycode] == KeyState::PRESSED;
}

bool Input::GetKeyUp(int keycode)
{
	if (keycode < 0 || static_cast<ui32>(keycode) >= keyStateCount)
		return false;
	return keyStates[keycode] == KeyState::RELEASED;
}
