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

	prop = coordinator.CreateEntity();
	Transform propTransform;
	propTransform.position.y = static_cast<real>(-1.5);
	// BoomBox.obj is modeled at real-world scale (~0.02m across, glTF's convention) - scaled
	// up here rather than re-exporting the mesh, so it reads at roughly the same size the old
	// SK_Bio_Mutant placeholder did at this camera distance.
	propTransform.scale = Vec3UnitScale * static_cast<real>(80.0);
	coordinator.AddComponent(prop, propTransform);
	coordinator.AddComponent(prop, MeshRenderer());
	coordinator.GetComponent<MeshRenderer>(prop).SetMesh(*renderer, Mesh::LoadOBJ("./data/mdls/BoomBox/BoomBox.obj"));
	// CC0, from Khronos' glTF-Sample-Assets (Models/BoomBox) - see data/mdls/BoomBox/CREDITS.md.
	BsdfMaterial boomBoxMaterial(*renderer);
	boomBoxMaterial.albedo = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_albedo.png", false);
	boomBoxMaterial.metallic = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_metallic.png", false);
	boomBoxMaterial.roughness = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_roughness.png", false);
	boomBoxMaterial.ambientOcclusion = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_occlusion.png", false);
	boomBoxMaterial.normal = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_normal.png", false);
	boomBoxMaterial.emission = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_emissive.png", false);
	coordinator.GetComponent<MeshRenderer>(prop).materials.push_back(boomBoxMaterial);

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
		t.rotation = QuaternionFromEuler({ static_cast<real>(45.0),Time::time * static_cast<real>(22.5), static_cast<real>(0.0) });


		Transform& t2 = coordinator.GetComponent<Transform>(prop);
		t2.rotation = glm::angleAxis(glm::radians(Time::time * static_cast<real>(-22.5)), Vec3UnitY);
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
