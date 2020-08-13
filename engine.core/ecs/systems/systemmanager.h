#pragma once
#include "system.h"
#include "../entities/entitymanager.h"
#include <unordered_map>
#include <memory>
namespace Engine::ECS
{
	class SystemManager
	{
	public:
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			ui32 typeID = T::GetTypeID();

			auto system = std::make_shared<T>();

			systems.insert({ typeID, system });
			return system;
		}

		template<typename T>
		void SetSignature(Signature signature)
		{
			ui32 typeID = T::GetTypeID();

			signatures.insert({ typeID, signature });
		}

		void EntityDestroyed(Entity entity)
		{
			for (auto const& pair : systems)
			{
				auto const& system = pair.second;

				system->entities.erase(entity);
			}
		}
		void EntitySignatureChanged(Entity entity, Signature entitySignature)
		{
			for (auto const& pair : systems)
			{
				auto const& type = pair.first;
				auto const& system = pair.second;
				auto const& systemSig = signatures[type];

				if ((entitySignature & systemSig) == systemSig)
				{
					system->entities.insert(entity);
				}
				else
				{
					system->entities.erase(entity);
				}
			}
		}
	private:
		std::unordered_map<ui32, Signature> signatures{};

		std::unordered_map<ui32, std::shared_ptr<System>> systems{};
	};
}