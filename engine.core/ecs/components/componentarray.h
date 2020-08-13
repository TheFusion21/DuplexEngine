#include "../entities/entity.h"
#include "component.h"
#include <unordered_map>
#include <array>
namespace Engine::ECS
{
	template<typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		void InsertData(Entity entity, T component)
		{
			size_t newIndex = size;
			entityToIndexMap[entity] = newIndex;
			indexToEntityMap[newIndex] = entity;
			componentArray[newIndex] = component;
			++size;
		}
		void RemoveData(Entity entity)
		{
			size_t indexOfRemovedEntity = entityToIndexMap[entity];
			size_t indexOfLast = size - 1;
			componentArray[indexOfRemovedEntity] = componentArray[indexOfLast];

			Entity entityOfLast = indexToEntityMap[indexOfLast];
			entityToIndexMap[entityOfLast] = indexOfRemovedEntity;
			indexToEntityMap[indexOfRemovedEntity] = entityOfLast;

			entityToIndexMap.erase(entity);
			indexToEntityMap.erase(indexOfLast);

			--size;
		}
		T& GetData(Entity entity)
		{
			return componentArray[entityToIndexMap[entity]];
		}

		void EntityDestroyed(Entity entity)
		{
			if (entityToIndexMap.find(entity) != entityToIndexMap.end())
			{
				RemoveData(entity);
			}
		}
	private:
		std::array<T, MAX_ENTITIES> componentArray{};

		std::unordered_map<Entity, size_t> entityToIndexMap{};

		std::unordered_map<size_t, Entity> indexToEntityMap{};

		size_t size = 0;
	};
}