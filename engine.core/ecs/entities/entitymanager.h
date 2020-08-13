#pragma once
#include "entity.h"
#include <bitset>
#include <queue>
#include <array>
namespace Engine::ECS
{
	using Signature = std::bitset<MAX_COMPONENTS>;

	class EntityManager
	{
	public:
		EntityManager()
		{
			for (ui32 ent = 0; ent < MAX_ENTITIES; ent++)
			{
				availableIds.push(ent);
			}
		}
		Entity CreateEntity()
		{
			Entity ent = availableIds.front();
			availableIds.pop();
			++livingEntityCount;

			return ent;
		}
		void DestroyEntity(Entity entity)
		{

			signatures[entity].reset();

			availableIds.push(entity);
			--livingEntityCount;
		}

		void SetSignature(Entity entity, Signature signature)
		{
			signatures[entity] = signature;
		}
		Signature GetSignature(Entity entity)
		{
			return signatures[entity];
		}
	private:
		std::queue<Entity> availableIds{};

		std::array<Signature, MAX_ENTITIES> signatures{};
		ui32 livingEntityCount = 0;
	};
}