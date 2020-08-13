#include "component.h"
#include "componentarray.h"
#include <unordered_map>
#include <array>
#include <memory>
namespace Engine::ECS
{
	class ComponentManager
	{
	public:
		template<typename T>
		void RegisterComponent()
		{
			ui32 typeID = T::GetTypeID();

			componentTypes.insert({ typeID, nextComponentType });

			componentArrays.insert({ typeID, std::make_shared<ComponentArray<T>>() });

			++nextComponentType;
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			ui32 typeID = T::GetTypeID();

			return componentTypes[typeID];
		}
		template<typename T>
		void AddComponent(Entity entity, T component)
		{
			GetComponentArray<T>()->InsertData(entity, component);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		void EntityDestroyed(Entity entity)
		{
			for (auto const& pair : componentArrays)
			{
				auto const& component = pair.second;

				component->EntityDestroyed(entity);
			}
		}
	private:
		std::unordered_map<ui32, ComponentType> componentTypes{};

		std::unordered_map<ui32, std::shared_ptr<IComponentArray>> componentArrays{};

		ComponentType nextComponentType{};

		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			ui32 typeID = T::GetTypeID();

			return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeID]);
		}
	};
}