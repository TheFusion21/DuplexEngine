#pragma once

// EXTERNAL INCLUDES
// INTERNAL INCLUDES


class GameObject;

namespace Game::Client
{
	class Application;
}
namespace Engine::Components
{

	enum class ComponentType
	{
		Invalid,
		Camera,
		MeshRenderer,
		Transform,
		FlyCamera
	};
	class Component
	{
	public:
		static ComponentType baseType;
		ComponentType type;
		GameObject* const gameObject = nullptr;
	protected:
		Component(GameObject* attach, ComponentType type);
		friend class Game::Client::Application;
		virtual void Update() {}
	};
}
