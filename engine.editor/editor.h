#pragma once
// EXTERNAL INCLUDES
#include <string>
#include <memory>
// INTERNAL INCLUDES
#include "math/types.h"
#include "window.h"
#include "renderer.h"
#include "namespaces.h"
#include "engineconfig.h"

namespace DUPLEX_NS_EDITOR
{
	class Editor
	{
	private:
		// Decided once at Init() (matching how main.cpp is launched: with or without a project
		// file argument), but OpenProject() can move a running ProjectHub instance into Editing
		// without restarting the process (see the Hub UI's "Open" button).
		enum class Mode
		{
			ProjectHub,
			Editing
		};
		Mode mode = Mode::ProjectHub;
		std::string currentProjectFile;

		DUPLEX_NS_CONFIG::EngineConfig config;
		bool isInit = false;
		DUPLEX_NS_WINDOW::Window window;
		std::unique_ptr<DUPLEX_NS_GRAPHICS::Renderer> renderer;

		bool InitEditor(std::string file);
		bool InitProjectExplorer();
		bool InitRendererAndImGui(const char* title, ui32 width, ui32 height);
		void OpenProject(const std::string& file);
		void DrawProjectHubUI();
		void DrawEditorUI();
	public:
		bool Init(std::string file);
		void Run();
		void Shutdown();
		std::string OpenFileDialog(std::string extFilter);
	};
}
