#pragma once
// EXTERNAL INCLUDES
#include <string>

// INTERNAL INCLUDES
#include "math/types.h"
#include "window.h"
#include "namespaces.h"
#include "engineconfig.h"

namespace DUPLEX_NS_EDITOR
{
	class Editor
	{
	private:
		DUPLEX_NS_CONFIG::EngineConfig config;
		bool isInit = false;
		DUPLEX_NS_WINDOW::Window window;
		bool InitEditor(std::string file);
		bool InitProjectExplorer();
	public:
		bool Init(std::string file);
		void Run();
		void Shutdown();
		std::string OpenFileDialog(std::string extFilter);
	};
}
