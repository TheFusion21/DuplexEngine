#include "transform.h"

using namespace Engine::Components;
using namespace Engine::Math;

ComponentType Transform::baseType = ComponentType::Transform;

Transform::Transform(GameObject* attach) : Component(attach, baseType)
{
	position = Vec3::Zero;
	rotation = Quaternion::Zero;
	scale = Vec3::UnitScale;
}