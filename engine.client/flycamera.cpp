#include "flycamera.h"
#include "input.h"
#include "time.h"
using namespace Engine::Components;
using namespace Engine::Math;
using namespace Engine::Utils;

ComponentType FlyCamera::baseType = ComponentType::FlyCamera;

void FlyCamera::Update()
{
	if (Input::GetKey(0x41))
	{
		gameObject->transform->position += -gameObject->transform->rotation * -Vec3::UnitX * Time::deltaTime * speed;
	}
	if (Input::GetKey(0x44))
	{
		gameObject->transform->position += -gameObject->transform->rotation * Vec3::UnitX * Time::deltaTime * speed;
	}
	if (Input::GetKey(0x57))
	{
		gameObject->transform->position += -gameObject->transform->rotation * -Vec3::UnitZ * Time::deltaTime * speed;
	}
	if (Input::GetKey(0x53))
	{
		gameObject->transform->position += -gameObject->transform->rotation * Vec3::UnitZ * Time::deltaTime * speed;
	}
	if (Input::GetKey(0x51))
	{
		gameObject->transform->position += -gameObject->transform->rotation * Vec3::UnitY * Time::deltaTime * speed;
	}
	if (Input::GetKey(0x45))
	{
		gameObject->transform->position += -gameObject->transform->rotation * -Vec3::UnitY * Time::deltaTime * speed;
	}
	if (Input::GetKey(0xA0))
	{
		speed = 5.f;
	}
	else
	{
		speed = 2.f;
	}

	Vec2 deltaMouse = oldMousePosition - Input::mousePosition;
	if (Input::GetKey(0x01))
	{
		rotX -= deltaMouse.x * .25f;
		rotY -= deltaMouse.y * .25f;
	}
	oldMousePosition = Input::mousePosition;
	gameObject->transform->rotation = Quaternion::FromAngleAxis(rotY, Vec3::UnitX) * Quaternion::FromAngleAxis(rotX, Vec3::UnitY);
}

FlyCamera::FlyCamera(GameObject* attach) : Component(attach, baseType)
{
	oldMousePosition = Input::mousePosition;
}
