#pragma once
#include <string>
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
		static DUPLEX_NS_MATH::Vec2 _mousePosition;
	public:
		static const DUPLEX_NS_MATH::Vec2& mousePosition;
		/// <summary>
		/// Initilize Input
		/// </summary>
		static void Init();
		/// <summary>
		/// Updates the keyStates
		/// </summary>
		static void Update();
		/// <summary>
		/// Check if key is currently held down
		/// </summary>
		/// <param name="keycode">the key to check for</param>
		/// <returns>is held down</returns>
		static bool GetKey(int keycode);
		/// <summary>
		/// Check if a key was pressed down this frame
		/// </summary>
		/// <param name="keycode">the key to check for</param>
		/// <returns>was pressed</returns>
		static bool GetKeyDown(int keycode);
		/// <summary>
		/// Check if a key was released down this frame
		/// </summary>
		/// <param name="keycode">the key to check for</param>
		/// <returns>was released</returns>
		static bool GetKeyUp(int keycode);

	};
}