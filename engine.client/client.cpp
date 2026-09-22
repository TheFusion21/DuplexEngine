// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "client.h"
#include "math/types.h"
#if defined(_WIN32)
#include "d3d11renderer.h"
#else
#include "vulkanrenderer.h"
#endif
#include "enginetime.h"
#include "input/input.h"
#include "graphics/light.h"
#include "mesh.h"
using namespace Game::Client;
using namespace DUPLEX_NS_GRAPHICS;
using namespace DUPLEX_NS_MATH;
using namespace DUPLEX_NS_UTIL;
using namespace DUPLEX_NS_RESOURCES;
using namespace Engine::ECS;

void Application::Init()
{
	AnsiString name = "PR210 Engine";
#if defined(_WIN32)
	bool windowReady = window.Init(name, 1280, 720);
#else
	bool windowReady = window.Init(name, 1280, 720, true);
#endif
	if (!windowReady)
	{
		return;
	}
	window.Show();
	window.SetTitle("THIS IS A Engine");
#if defined(_WIN32)
	renderer = std::make_unique<D3D11Renderer>();
#else
	renderer = std::make_unique<VulkanRenderer>();
#endif
	ui32 width, height;
	window.GetClientSize(width, height);
	if (!renderer->Init(window.GetSDLWindow(), width, height))
	{
		return;
	}
	window.SetRenderer(renderer.get());
	coordinator.Init();
	Input::Init();

	//Register required Components
	coordinator.RegisterComponent<Transform>();
	coordinator.RegisterComponent<Camera>();
	coordinator.RegisterComponent<MeshRenderer>();
	coordinator.RegisterComponent<Light>();
	//Register required Systems
	camSystem = coordinator.RegisterSystem<CameraSystem>();
	meshSystem = coordinator.RegisterSystem<MeshSystem>();
	lightSystem = coordinator.RegisterSystem<LightSystem>();

	//Signature required Components for Systems
	Signature camSignature;
	camSignature.set(coordinator.GetComponentType<Transform>());
	camSignature.set(coordinator.GetComponentType<Camera>());
	coordinator.SetSystemSignature<CameraSystem>(camSignature);

	Signature meshSignature;
	meshSignature.set(coordinator.GetComponentType<Transform>());
	meshSignature.set(coordinator.GetComponentType<MeshRenderer>());
	coordinator.SetSystemSignature<MeshSystem>(meshSignature);

	Signature lightSignature;
	lightSignature.set(coordinator.GetComponentType<Transform>());
	lightSignature.set(coordinator.GetComponentType<Light>());
	coordinator.SetSystemSignature<LightSystem>(lightSignature);

	Entity camera = coordinator.CreateEntity();
	Transform camTransform;
	camTransform.position.z = static_cast<real>(3.0);
	coordinator.AddComponent(camera, camTransform);
	coordinator.AddComponent(camera, Camera());

	cube = coordinator.CreateEntity();
	Transform cubeTransform;
	cubeTransform.position.y = static_cast<real>(-1.5);
	cubeTransform.scale = Vec3::UnitScale * static_cast<real>(2.0);
	coordinator.AddComponent(cube, cubeTransform);
	coordinator.AddComponent(cube, MeshRenderer());
	coordinator.GetComponent<MeshRenderer>(cube).SetMesh(*renderer, Mesh::LoadOBJ("./data/mdls/SK_Bio_Mutant.obj"));
	BsdfMaterial uv_1(*renderer);
	uv_1.albedo = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_a.png", false);
	uv_1.metallic = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_m.png", false);
	uv_1.roughness = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_rg.png", false);
	uv_1.ambientOcclusion = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_AO.png", false);
	uv_1.normal = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_n.png", false);
	uv_1.emission = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Top_emissive.png", false);
	coordinator.GetComponent<MeshRenderer>(cube).materials.push_back(uv_1);
	BsdfMaterial uv_2(*renderer);
	uv_2.albedo = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_a.png", false);
	uv_2.metallic = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_m.png", false);
	uv_2.roughness = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_rg.png", false);
	uv_2.ambientOcclusion = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_AO.png", false);
	uv_2.normal = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_n.png", false);
	uv_2.emission = Texture2D::LoadFromFile(*renderer, "./data/mdls/Skin_1/T_Biomech_Mutant_Skin_1_Bottom_emissive.png", false);
	coordinator.GetComponent<MeshRenderer>(cube).materials.push_back(uv_2);

	dirLight = coordinator.CreateEntity();
	coordinator.AddComponent(dirLight, Light());
	Transform lightTransform;
	coordinator.AddComponent(dirLight, lightTransform);
	Time::Start();
	this->appState = AppState::Running;
}

void Application::Run()
{
	while(this->appState == AppState::Running)
	{
		camSystem->Update(coordinator, *renderer);
		lightSystem->Update(coordinator, *renderer);

		renderer->BeginScene();
		meshSystem->Update(coordinator, *renderer);
		renderer->EndScene();
		if (!window.PollEvents())
		{
			this->appState = AppState::Stopped;
		}
		Input::Update();
		Transform& t = coordinator.GetComponent<Transform>(dirLight);
		t.rotation = Quaternion::FromEuler({ static_cast<real>(45.0),Time::time * static_cast<real>(22.5), static_cast<real>(0.0) });


		Transform& t2 = coordinator.GetComponent<Transform>(cube);
		t2.rotation = Quaternion::FromAngleAxis(Time::time * static_cast<real>(-22.5), Vec3::UnitY);
		Time::Update();
	}
}

void Application::Shutdown()
{
	// renderer can still be null here: main() always calls Shutdown() after Init(), even if
	// Init() bailed out before constructing it (e.g. window.Init() failing).
	if (renderer)
	{
		renderer->Shutdown();
	}
	window.Shutdown();
}
