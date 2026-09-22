#pragma once
#include "math/vec2.h"

namespace DUPLEX_NS_UTIL
{
	class Input
	{
	private:
		Input() {}
		enum class KeyState
		{
			NONE,
			PRESSED,
			DOWN,
			RELEASED,
		};
		static KeyState* keyStates;
		static ui32 keyStateCount;
		static DUPLEX_NS_MATH::Vec2 _mousePosition;
	public:
		static const DUPLEX_NS_MATH::Vec2& mousePosition;
		/// <summary>
		/// Initilize Input
		/// </summary>
		static void Init();
		/// <summary>
		/// Updates the keyStates. Call once per frame, after the window has pumped its event
		/// queue for that frame (see DUPLEX_NS_WINDOW::Window::PollEvents).
		/// </summary>
		static void Update();
		/// <summary>
		/// Check if key is currently held down
		/// </summary>
		/// <param name="keycode">the key to check for, as an SDL_Scancode (see SDL_scancode.h)</param>
		/// <returns>is held down</returns>
		static bool GetKey(int keycode);
		/// <summary>
		/// Check if a key was pressed down this frame
		/// </summary>
		/// <param name="keycode">the key to check for, as an SDL_Scancode (see SDL_scancode.h)</param>
		/// <returns>was pressed</returns>
		static bool GetKeyDown(int keycode);
		/// <summary>
		/// Check if a key was released down this frame
		/// </summary>
		/// <param name="keycode">the key to check for, as an SDL_Scancode (see SDL_scancode.h)</param>
		/// <returns>was released</returns>
		static bool GetKeyUp(int keycode);

	};
}
