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
	Input::Init();

	entt::entity camera = registry.create();
	Transform camTransform;
	camTransform.position.z = static_cast<real>(3.0);
	registry.emplace<Transform>(camera, camTransform);
	registry.emplace<Camera>(camera);

	prop = registry.create();
	Transform propTransform;
	propTransform.position.y = static_cast<real>(-1.5);
	// BoomBox.obj is modeled at real-world scale (~0.02m across, glTF's convention) - scaled
	// up here rather than re-exporting the mesh, so it reads at roughly the same size the old
	// SK_Bio_Mutant placeholder did at this camera distance.
	propTransform.scale = Vec3UnitScale * static_cast<real>(80.0);
	registry.emplace<Transform>(prop, propTransform);
	registry.emplace<MeshRenderer>(prop);
	registry.get<MeshRenderer>(prop).SetMesh(*renderer, Mesh::LoadOBJ("./data/mdls/BoomBox/BoomBox.obj"));
	// CC0, from Khronos' glTF-Sample-Assets (Models/BoomBox) - see data/mdls/BoomBox/CREDITS.md.
	BsdfMaterial boomBoxMaterial(*renderer);
	boomBoxMaterial.albedo = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_albedo.png", false);
	boomBoxMaterial.metallic = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_metallic.png", false);
	boomBoxMaterial.roughness = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_roughness.png", false);
	boomBoxMaterial.ambientOcclusion = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_occlusion.png", false);
	boomBoxMaterial.normal = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_normal.png", false);
	boomBoxMaterial.emission = Texture2D::LoadFromFile(*renderer, "./data/mdls/BoomBox/BoomBox_emissive.png", false);
	registry.get<MeshRenderer>(prop).materials.push_back(boomBoxMaterial);

	dirLight = registry.create();
	registry.emplace<Light>(dirLight);
	Transform lightTransform;
	registry.emplace<Transform>(dirLight, lightTransform);
	Time::Start();
	this->appState = AppState::Running;
}

void Application::Run()
{
	while(this->appState == AppState::Running)
	{
		CameraSystem::Update(registry, *renderer);
		LightSystem::Update(registry, *renderer);

		renderer->BeginScene();
		MeshSystem::Update(registry, *renderer);
		renderer->EndScene();
		if (!window.PollEvents())
		{
			this->appState = AppState::Stopped;
		}
		Input::Update();
		Transform& t = registry.get<Transform>(dirLight);
		t.rotation = QuaternionFromEuler({ static_cast<real>(45.0),Time::time * static_cast<real>(22.5), static_cast<real>(0.0) });


		Transform& t2 = registry.get<Transform>(prop);
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
