// EXTERNAL INCLUDES
#include <cstdio>
// INTERNAL INCLUDES
#include "client.h"
int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	Game::Client::Application app { };

	app.Init();
	app.Run();
	app.Shutdown();

	
	return 0;
}