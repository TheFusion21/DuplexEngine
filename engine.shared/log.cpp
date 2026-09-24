#include "log.h"
// EXTERNAL INCLUDES
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>

using namespace DUPLEX_NS_LOG;

namespace
{
	bool initialized = false;

	std::shared_ptr<spdlog::logger> coreLogger;
	std::shared_ptr<spdlog::logger> rendererLogger;
	std::shared_ptr<spdlog::logger> physicsLogger;
	std::shared_ptr<spdlog::logger> audioLogger;
	std::shared_ptr<spdlog::logger> ecsLogger;
	std::shared_ptr<spdlog::logger> editorLogger;
	std::shared_ptr<spdlog::logger> windowLogger;

	std::shared_ptr<spdlog::logger> MakeCategoryLogger(const char* name, const std::vector<spdlog::sink_ptr>& sinks)
	{
		auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
		logger->set_level(spdlog::level::trace);
		// Anything warn-or-worse hits disk immediately rather than waiting on spdlog's usual
		// buffering - the cases most worth surviving a crash the log line itself is reporting.
		logger->flush_on(spdlog::level::warn);
		spdlog::register_logger(logger);
		return logger;
	}
}

void Logger::Init()
{
	if (initialized)
		return;

	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	consoleSink->set_pattern("%^[%H:%M:%S.%e] [%n] [%l]%$ %v");

	// 5 MiB x 3 rotated files - plenty for a dev session, small enough to not matter. bin/ is
	// entirely gitignored already (build output lives there), so logs/ living alongside it
	// needs no separate gitignore entry. spdlog creates the directory itself if missing.
	auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		"bin/logs/duplex.log", 5 * 1024 * 1024, 3);
	fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

	const std::vector<spdlog::sink_ptr> sharedSinks = { consoleSink, fileSink };

	coreLogger = MakeCategoryLogger("Core", sharedSinks);
	rendererLogger = MakeCategoryLogger("Renderer", sharedSinks);
	physicsLogger = MakeCategoryLogger("Physics", sharedSinks);
	audioLogger = MakeCategoryLogger("Audio", sharedSinks);
	ecsLogger = MakeCategoryLogger("ECS", sharedSinks);
	editorLogger = MakeCategoryLogger("Editor", sharedSinks);
	windowLogger = MakeCategoryLogger("Window", sharedSinks);

	spdlog::set_default_logger(coreLogger);
	initialized = true;
}

void Logger::Shutdown()
{
	if (!initialized)
		return;

	// Drops every registered logger (flushing each first) and destroys the sinks with them -
	// the individual shared_ptrs above are left dangling references into freed loggers after
	// this, but nothing touches them again between here and process exit.
	spdlog::shutdown();
	initialized = false;
}

spdlog::logger& Logger::Core() { return *coreLogger; }
spdlog::logger& Logger::Renderer() { return *rendererLogger; }
spdlog::logger& Logger::Physics() { return *physicsLogger; }
spdlog::logger& Logger::Audio() { return *audioLogger; }
spdlog::logger& Logger::ECS() { return *ecsLogger; }
spdlog::logger& Logger::Editor() { return *editorLogger; }
spdlog::logger& Logger::Window() { return *windowLogger; }
