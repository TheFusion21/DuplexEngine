
#include "editor.h"

#include <iostream>

Duplex::Editor::Editor editor;

int main(int argc, char** argv)
{
	
	std::string filename;
	if (argc == 2)
	{
		filename = argv[1];
	}
	if (!editor.Init(filename))
		return 0;
	editor.Run();
	editor.Shutdown();
	return 0;
}