#pragma once

// EXTERNAL INCLUDES
#include <memory>
// INTERNAL INCLUDES
#include "appwindow.h"
#include "renderer.h"
#include "ecs/coordinator.h"
#include "ecs/systems/camerasystem.h"
#include "ecs/systems/meshsystem.h"
#include "ecs/systems/lightsystem.h"
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

		// Owns the backend; AppWindow gets a non-owning pointer (see SetRenderer) so its
		// WM_SIZE/fullscreen handling can reach it without a global.
		std::unique_ptr<DUPLEX_NS_GRAPHICS::Renderer> renderer;

		Engine::ECS::Coordinator coordinator;
		std::shared_ptr<Engine::ECS::CameraSystem> camSystem;
		std::shared_ptr<Engine::ECS::MeshSystem> meshSystem;
		std::shared_ptr<Engine::ECS::LightSystem> lightSystem;
		Engine::ECS::Entity cube;
		Engine::ECS::Entity dirLight;

	public:
		void Init();
		void Run();
		void Shutdown();
	};
}
