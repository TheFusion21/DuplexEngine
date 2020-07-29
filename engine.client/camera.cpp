// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "camera.h"
#include "gameobject.h"

using namespace Engine::Math;
using namespace Engine::Components;

Camera* Camera::activeCamera = nullptr;

ComponentType Camera::baseType = ComponentType::Camera;

Camera::Camera(GameObject* attach) : Component(attach, baseType)
{
	this->viewProjMatrix = Mat4x4::Identity;
}

void Camera::Update()
{
	this->viewProjMatrix = Mat4x4::FromPerspectiveFOV(this->h_fov, this->aspect, this->nearPlane, this->farPlane) * Mat4x4::FromOrientation(gameObject->transform->rotation) * Mat4x4::FromView(gameObject->transform->position);
}

void Camera::SetFov(real h_fov)
{
	this->h_fov = h_fov;
}

void Camera::SetAspect(real aspect)
{
	this->aspect = aspect;
}

void Camera::SetPlanes(real nearPlane, real farPlane)
{
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
}
