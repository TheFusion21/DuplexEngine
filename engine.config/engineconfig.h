#pragma once
// EXTERNAL INCLUDES
#include <vector>
#include <string>
// INTERNAL INCLUDES
#include "namespaces.h"

namespace DUPLEX_NS_CONFIG
{
	const std::string configFileName = "EngineConfig.ini";
#if defined(_WINDOWS)
	const std::string fileFilter = "Duplex Project\0*.dproj\0\0";
#endif
	struct EngineConfig
	{
		std::vector<std::string> lastProjectFiles;
	};
	const EngineConfig defaultConfig =
	{
		{},
	};
}
