#pragma once
#include "../entities/entity.h"

namespace Engine::ECS
{
	const ui32 MAX_COMPONENTS = 1024;

	using ComponentType = ui32;
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};
}