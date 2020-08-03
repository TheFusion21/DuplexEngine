#include "Input.h"
#include "utils/Util.h"
#include <windows.h>
#include <iostream>


using namespace Engine::Utils;

Input::KeyState* Input::keyStates;

Engine::Math::Vec2 Input::_mousePosition = Engine::Math::Vec2::Zero;
const Engine::Math::Vec2& Input::mousePosition = Input::_mousePosition;

void Input::Init()
{
	keyStates = new KeyState[0xFF];
	for (int i = 0x01; i < 0xFF; i++)
	{
		keyStates[i] = KeyState::NONE;
	}
	POINT pos;
	GetCursorPos(&pos);
	_mousePosition.x = static_cast<real>(pos.x);
	_mousePosition.y = static_cast<real>(pos.y);
}

void Input::Update()
{
	POINT pos;
	GetCursorPos(&pos);
	_mousePosition.x = static_cast<real>(pos.x);
	_mousePosition.y = static_cast<real>(pos.y);
	for (int i = 0x01; i < 0xFF; i++)
	{
		if (keyStates[i] == KeyState::PRESSED)
			keyStates[i] = KeyState::DOWN;
		else if (keyStates[i] == KeyState::RELEASED)
			keyStates[i] = KeyState::NONE;
	}
	for (int i = 0x01; i < 0xFF; i++)
	{
		if ((::GetAsyncKeyState(i) & 0x8000) != 0)
		{
			//std::cout << "is down" << std::endl;
			//std::cout << "ks is: " << (keyStates[i] == KeyState::NONE ? "NONE" : (keyStates[i] == KeyState::PRESSED ? "PRESSED" : (keyStates[i] == KeyState::DOWN ? "DOWN" : "RELEASED"))) << std::endl;
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
	return (keyStates[keycode] == KeyState::PRESSED || keyStates[keycode] == KeyState::DOWN);
}

bool Input::GetKeyDown(int keycode)
{
	return keyStates[keycode] == KeyState::PRESSED;
}

bool Input::GetKeyUp(int keycode)
{
	return keyStates[keycode] == KeyState::RELEASED;
}