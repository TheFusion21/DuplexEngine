// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "client.h"
#include "math/types.h"
#include "d3d11renderer.h"
#include "time.h"
#include "camera.h"
#include "meshrenderer.h"
#include "flycamera.h"
#include "input/input.h"

using namespace Game::Client;
using namespace Engine::Graphics;
using namespace Engine::Math;
using namespace Engine::Components;
using namespace Engine::Resources;
using namespace Engine::Utils;

void Application::Init()
{
	AnsiString name = "PR210 Engine";
	window.Init(name, 1280, 720);
	window.Show();
	Renderer::CreateInstance(new D3D11Renderer());
	if (!Renderer::GetInstancePtr()->Init(window))
	{
		return;
	}
	Input::Init();
	{
		gameObjects.push_back(new GameObject());
		
		Camera* camComponent = gameObjects[0]->AddComponent<Camera>();
		camComponent->SetFov(static_cast<real>(75.0));
		camComponent->SetPlanes(static_cast<real>(0.001), static_cast<real>(10000.0));
		Renderer::GetInstancePtr()->SetActiveCamera(camComponent);
		gameObjects[0]->AddComponent<FlyCamera>();
		gameObjects[0]->transform->position.z = static_cast<real>(3.0);
		
	}
	{
		gameObjects.push_back(new GameObject());
		gameObjects[1]->AddComponent<MeshRenderer>()->SetMesh(Mesh::GenerateFlatCube());
		Material m;
		m.roughness = 0.1f;
		m.Kd = { 1.f, 0.f, 0.f, 1.f };
		gameObjects[1]->GetComponent<MeshRenderer>()->SetMaterial(m);
		gameObjects[1]->transform->position.x = static_cast<real>(2.0);
	}
	{
		gameObjects.push_back(new GameObject());
		gameObjects[2]->AddComponent<MeshRenderer>()->SetMesh(Mesh::GenerateSphere(32, 64));
		Material m;
		m.roughness = 0.9f;
		m.Kd = { 0.8f, 0.8f, 0.1f, 1.f };
		gameObjects[2]->GetComponent<MeshRenderer>()->SetMaterial(m);
		gameObjects[2]->transform->scale = Vec3::UnitScale * static_cast<real>(1.6);
	}
	{
		gameObjects.push_back(new GameObject());
		gameObjects[3]->AddComponent<MeshRenderer>()->SetMesh(Mesh::GenerateQuad());
		gameObjects[3]->GetComponent<MeshRenderer>()->SetShaderType(MeshRenderer::ShaderType::SignedDistanceField);
		gameObjects[3]->GetComponent<MeshRenderer>()->UseTexture("./data/tex/font_test.png");
		gameObjects[3]->transform->position.x = -static_cast<real>(2.0);
		gameObjects[3]->transform->scale = Vec3::UnitScale * static_cast<real>(2.0);
	}
	
	Time::Start();
	this->appState = AppState::Running;
}

void Application::Run()
{
	while(this->appState == AppState::Running)
	{
		
		//RENDER
		Renderer::GetInstancePtr()->BeginScene();

		for (GameObject*& obj : gameObjects)
		{
			D3D11Renderer::GetInstancePtr()->Render(obj);
		}

		Renderer::GetInstancePtr()->EndScene();
		if (!window.MessagePump())
		{
			this->appState = AppState::Stopped;
		}

		//UPDATE
		for (GameObject*& obj : gameObjects)
		{
			for (Component*& cmpnt : obj->components)
			{
				cmpnt->Update();
			}
		}

		Time::Update();
		if(window.IsFocused())
			Input::Update();
		gameObjects[1]->transform->scale = Vec3::UnitScale * (Time::sinTime * static_cast<real>(0.25) + static_cast<real>(1.0));
		gameObjects[1]->transform->rotation = Quaternion::FromAngleAxis(Time::time * static_cast<real>(30.0), Vec3::UnitY * static_cast<real>(Time::sinTime) - Vec3::UnitX);
	}
}

void Application::Shutdown()
{
	Renderer::GetInstancePtr()->Shutdown();
}
