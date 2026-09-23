#pragma once

// EXTERNAL INCLUDES
#include <memory>
#include <entt/entt.hpp>
// INTERNAL INCLUDES
#include "window.h"
#include "renderer.h"
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

		entt::registry registry;
		entt::entity prop;
		entt::entity dirLight;

	public:
		void Init();
		void Run();
		void Shutdown();
	};
}
