// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "client.h"

int main(int argc, char** argv)
{
	Game::Client::Application app { };

	app.Init();
	app.Run();
	app.Shutdown();
	
	return 0;
}
