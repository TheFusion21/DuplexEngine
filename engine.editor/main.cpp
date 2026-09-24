
#include "editor.h"
#include "log.h"

#include <iostream>

Duplex::Editor::Editor editor;

int main(int argc, char** argv)
{
	DUPLEX_NS_LOG::Logger::Init();

	std::string filename;
	if (argc == 2)
	{
		filename = argv[1];
	}
	if (!editor.Init(filename))
	{
		DUPLEX_NS_LOG::Logger::Shutdown();
		return 0;
	}
	editor.Run();
	editor.Shutdown();
	DUPLEX_NS_LOG::Logger::Shutdown();
	return 0;
}
