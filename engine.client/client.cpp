// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "client.h"
#include "math/types.h"
#include "d3d11renderer.h"
#include "vulkanrenderer.h"
#include "time.h"
#include "input/input.h"

#include "graphics/material.h"
#include "mesh.h"
using namespace Game::Client;
using namespace Engine::Graphics;
using namespace Engine::Math;
using namespace Engine::Utils;
using namespace Engine::Resources;
using namespace Engine::ECS;

void Application::Init()
{
	AnsiString name = "PR210 Engine";
	window.Init(name, 1280, 720);
	window.Show();
	Renderer::CreateInstance(new D3D11Renderer());
	ui32 width, height;
	window.GetClientSize(width, height);
	if (!Renderer::GetInstancePtr()->Init(window.GetInstance(), window.GetHandle(), width, height))
	{
		return;
	}
	coordinator.Init();
	Input::Init();

	//Register required Components
	coordinator.RegisterComponent<Transform>();
	coordinator.RegisterComponent<Camera>();
	coordinator.RegisterComponent<MeshRenderer>();
	//Register required Systems
	camSystem = coordinator.RegisterSystem<CameraSystem>();
	meshSystem = coordinator.RegisterSystem<MeshSystem>();

	//Signature required Components for Systems
	Signature camSignature;
	camSignature.set(coordinator.GetComponentType<Transform>());
	camSignature.set(coordinator.GetComponentType<Camera>());
	coordinator.SetSystemSignature<CameraSystem>(camSignature);

	Signature meshSignature;
	meshSignature.set(coordinator.GetComponentType<Transform>());
	meshSignature.set(coordinator.GetComponentType<MeshRenderer>());
	coordinator.SetSystemSignature<MeshSystem>(meshSignature);


	Entity camera = coordinator.CreateEntity();
	Transform camTransform;
	camTransform.position.z = static_cast<real>(3.0);
	coordinator.AddComponent(camera, camTransform);
	coordinator.AddComponent(camera, Camera());

	Entity cube = coordinator.CreateEntity();
	Transform cubeTransform;
	cubeTransform.position.x = static_cast<real>(2.0);
	coordinator.AddComponent(cube, cubeTransform);
	coordinator.AddComponent(cube, MeshRenderer());
	Material m;
	m.roughness = 0.1f;
	m.Kd = { 1.f, 0.f, 0.f, 1.f };
	coordinator.GetComponent<MeshRenderer>(cube).SetMaterial(m);
	coordinator.GetComponent<MeshRenderer>(cube).SetMesh(Mesh::GenerateFlatCube());
	/*{
		gameObjects.push_back(new GameObject());
		
		//Camera* camComponent = gameObjects[0]->AddComponent<Camera>();
		//camComponent->SetFov(static_cast<real>(75.0));
		//camComponent->SetPlanes(static_cast<real>(0.001), static_cast<real>(10000.0));
		//gameObjects[0]->AddComponent<FlyCamera>();
		//gameObjects[0]->transform->position.z = static_cast<real>(3.0);
		
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
	*/
	Time::Start();
	this->appState = AppState::Running;
}

void Application::Run()
{
	while(this->appState == AppState::Running)
	{

		//Renderer::GetInstancePtr()->SetActiveCamera(gameObjects[0]->transform->position, gameObjects[0]->GetComponent<Camera>()->GetViewProjMatrix());
		//RENDER
		camSystem->Update(coordinator);
		
		Renderer::GetInstancePtr()->BeginScene();
		meshSystem->Update(coordinator);
		/*for (GameObject*& obj : gameObjects)
		{
			D3D11Renderer::GetInstancePtr()->Render(obj);
		}
		*/
		Renderer::GetInstancePtr()->EndScene();
		if (!window.MessagePump())
		{
			this->appState = AppState::Stopped;
		}
		/*
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
		*/
	}
}

void Application::Shutdown()
{
	Renderer::GetInstancePtr()->Shutdown();
}
