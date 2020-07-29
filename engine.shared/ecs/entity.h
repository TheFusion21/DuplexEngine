#pragma once

namespace Engine::Ecs
{
	class Entity
	{
	private:
		static unsigned int lastID;
	public:
		const unsigned int id;
		Entity();

	};
}