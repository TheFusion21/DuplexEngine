
// EXTERNAL INCLUDES
#if defined(_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#endif
#include <fstream>
#include <iostream>
#include <algorithm>
#include <imgui.h>
#include <imgui_impl_sdl.h>
// INTERNAL INCLUDES
#include "editor.h"
#if defined(_WIN32)
#include "d3d11renderer.h"
#else
#include "vulkanrenderer.h"
#endif

using namespace DUPLEX_NS_EDITOR;
using namespace DUPLEX_NS_WINDOW;
using namespace DUPLEX_NS_CONFIG;
using namespace DUPLEX_NS_GRAPHICS;

bool Editor::InitRendererAndImGui(const char* title, ui32 width, ui32 height)
{
#if defined(_WIN32)
	bool windowReady = window.Init(title, width, height);
#else
	bool windowReady = window.Init(title, width, height, true);
#endif
	if (!windowReady)
		return false;
	window.Show();

#if defined(_WIN32)
	renderer = std::make_unique<D3D11Renderer>();
#else
	renderer = std::make_unique<VulkanRenderer>();
#endif
	if (!renderer->Init(window.GetSDLWindow(), width, height))
		return false;
	window.SetRenderer(renderer.get());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	if (!renderer->InitImGui(window.GetSDLWindow()))
		return false;

	return true;
}

bool Editor::InitEditor(std::string file)
{
	if (!InitRendererAndImGui("Duplex Editor", 1600, 900))
		return false;
	OpenProject(file);
	return true;
}

bool Editor::InitProjectExplorer()
{
	return InitRendererAndImGui("Duplex Project Hub", 800, 500);
}

void Editor::OpenProject(const std::string& file)
{
	currentProjectFile = file;
	mode = Mode::Editing;

	auto existing = std::find(config.lastProjectFiles.begin(), config.lastProjectFiles.end(), file);
	if (existing != config.lastProjectFiles.end())
		config.lastProjectFiles.erase(existing);
	config.lastProjectFiles.insert(config.lastProjectFiles.begin(), file);
}

bool Editor::Init(std::string file)
{
	std::fstream configFile(configFileName, std::fstream::in);
	if (!configFile.good())
	{
		config = defaultConfig;
	}
	else
	{
		configFile.close();
	}
	isInit = file.empty() ? InitProjectExplorer() : InitEditor(file);
	return isInit;
}
void Editor::Run()
{
	bool running = true;
	while (running)
	{
		running = window.PollEvents([](const SDL_Event& event) {
			ImGui_ImplSDL2_ProcessEvent(&event);
		});

		renderer->BeginScene();
		renderer->ImGuiNewFrame(window.GetSDLWindow());

		if (mode == Mode::ProjectHub)
			DrawProjectHubUI();
		else
			DrawEditorUI();

		renderer->ImGuiRenderDrawData();
		renderer->EndScene();
	}
}
void Editor::Shutdown()
{
	// Checked independently (not via isInit) so a partial failure inside InitRendererAndImGui -
	// e.g. renderer->Init() succeeding but InitImGui() then failing - still cleans up whatever
	// was actually created, same reasoning as Game::Client::Application::Shutdown().
	if (renderer)
	{
		renderer->Shutdown();
	}
	if (ImGui::GetCurrentContext())
	{
		ImGui::DestroyContext();
	}

	// Save config file
	std::fstream configFile(configFileName, std::fstream::out);
	if (configFile.good())
	{
		configFile << "[General]" << std::endl;
		configFile << "lastProjects=(";
		for (int i = 0;i<(int)config.lastProjectFiles.size();i++)
		{
			configFile << config.lastProjectFiles[i];
			if (i < ((int)config.lastProjectFiles.size()) - 1)
				configFile << ",";
		}
		configFile << ")" << std::endl;

		configFile.close();
	}
	window.Shutdown();
}

void Editor::DrawProjectHubUI()
{
	// This ImGui version (see engine.editor/imgui, vendored pre-viewport-API) has no
	// ImGui::GetMainViewport() - io.DisplaySize is the portable equivalent for a single-window
	// app like this one.
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Duplex Project Hub", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("Duplex Project Hub");
	ImGui::Separator();

	if (ImGui::Button("Open Project..."))
	{
#if defined(_WINDOWS)
		std::string path = OpenFileDialog(fileFilter);
#else
		std::string path = OpenFileDialog(std::string());
#endif
		if (!path.empty())
			OpenProject(path);
	}
	ImGui::SameLine();
	if (ImGui::Button("New Project..."))
	{
		// Project creation (scaffolding a new .dproj + default folder layout) isn't
		// implemented yet - out of scope for wiring up the ImGui pipeline itself.
	}

	ImGui::Spacing();
	ImGui::Text("Recent Projects");
	ImGui::BeginChild("RecentProjects", ImVec2(0, 0), true);
	for (const std::string& path : config.lastProjectFiles)
	{
		if (ImGui::Selectable(path.c_str()))
		{
			OpenProject(path);
		}
	}
	ImGui::EndChild();

	ImGui::End();
}

void Editor::DrawEditorUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Close Project"))
			{
				mode = Mode::ProjectHub;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetFrameHeight()));
	ImGui::SetNextWindowSize(ImVec2(displaySize.x, displaySize.y - ImGui::GetFrameHeight()));
	ImGui::Begin("Duplex Editor", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::Text("Editing: %s", currentProjectFile.c_str());
	ImGui::TextDisabled("(scene hierarchy / inspector / viewport panels are not implemented yet)");
	ImGui::End();
}

std::string Editor::OpenFileDialog(std::string extFilter)
{
#if defined(_WINDOWS)
    char filename[MAX_PATH];
    OPENFILENAME ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter = extFilter.c_str();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a project file to open";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        return std::string(filename);
    }
#elif defined(_LINUX)

#endif
    return std::string();
}
