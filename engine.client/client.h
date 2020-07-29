#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "appwindow.h"
#include "gameobject.h"
#include "mesh.h"

namespace Game::Client
{
	class Application
	{
	private:
		Engine::Core::AppWindow window;
		enum class AppState
		{
			Started,
			Running,
			Stopped
		} appState = AppState::Started;

		std::vector<GameObject*> gameObjects;

	public:
		void Init();
		void Run();
		void Shutdown();
	};
}
