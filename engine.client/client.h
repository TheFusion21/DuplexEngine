#pragma once

// EXTERNAL INCLUDES
#include <memory>
// INTERNAL INCLUDES
#include "window.h"
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
		DUPLEX_NS_WINDOW::Window window;
		enum class AppState
		{
			Started,
			Running,
			Stopped
		} appState = AppState::Started;

		// Owns the backend; Window gets a non-owning pointer (see SetRenderer) so its resize
		// handling in PollEvents() can reach it without a global.
		std::unique_ptr<DUPLEX_NS_GRAPHICS::Renderer> renderer;

		Engine::ECS::Coordinator coordinator;
		std::shared_ptr<Engine::ECS::CameraSystem> camSystem;
		std::shared_ptr<Engine::ECS::MeshSystem> meshSystem;
		std::shared_ptr<Engine::ECS::LightSystem> lightSystem;
		Engine::ECS::Entity prop;
		Engine::ECS::Entity dirLight;

	public:
		void Init();
		void Run();
		void Shutdown();
	};
}
