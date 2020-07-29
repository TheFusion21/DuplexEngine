#pragma once

// EXTERNAL INCLUDES
#include <vector>
// INTERNAL INCLUDES
#include "transform.h"

class GameObject
{
public:
	Engine::Components::Transform*const transform;

	GameObject();

	template<class T>
	T* AddComponent();

	template<class T>
	T* GetComponent() const;
private:
	std::vector<Engine::Components::Component*> components;
	Engine::Components::Transform * _transform;
	friend class Game::Client::Application;
};

template<class T>
T* GameObject::GetComponent() const
{
	T* foundComponent = nullptr;
	for (auto& cmpnt : components)
	{
		foundComponent = static_cast<T*>(cmpnt);
		if (foundComponent != nullptr)
		{
			if (cmpnt->type == T::baseType)
			{
				return foundComponent;
			}
		}
	}
	return nullptr;
}
template<class T>
T* GameObject::AddComponent()
{
	components.push_back(static_cast<Engine::Components::Component*>(new T(this)));
	return GetComponent<T>();
}