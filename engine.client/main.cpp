// EXTERNAL INCLUDES
#include <cstdio>
// INTERNAL INCLUDES
#include "client.h"
#include "log.h"
int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	DUPLEX_NS_LOG::Logger::Init();
	Game::Client::Application app { };

	app.Init();
	app.Run();
	app.Shutdown();

	DUPLEX_NS_LOG::Logger::Shutdown();
	return 0;
}
