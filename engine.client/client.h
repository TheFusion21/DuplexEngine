#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "appwindow.h"
#include "ecs/coordinator.h"
#include "ecs/systems/camerasystem.h"
#include "ecs/systems/meshsystem.h"
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

		Engine::ECS::Coordinator coordinator;
		std::shared_ptr<Engine::ECS::CameraSystem> camSystem;
		std::shared_ptr<Engine::ECS::MeshSystem> meshSystem;
		
	public:
		void Init();
		void Run();
		void Shutdown();
	};
}
