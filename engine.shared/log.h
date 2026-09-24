#pragma once
// EXTERNAL INCLUDES
#include <spdlog/spdlog.h>
// INTERNAL INCLUDES
#include "namespaces.h"

namespace DUPLEX_NS_LOG
{
	// Thin wrapper around spdlog giving every engine subsystem its own named logger, so output
	// can be filtered per-system once there's enough of it to need filtering (see docs/roadmap
	// Phase 10). Every category shares the same two sinks under the hood (colored console,
	// rotating file at bin/logs/duplex.log) rather than each opening its own file.
	//
	// Init() must run once before any accessor below is used - main() does this first, right
	// after process start, for both engine.client and engine.editor. Shutdown() flushes and
	// tears every logger/sink down; call it last, after everything that might still log.
	//
	// Named Logger, not Log - a class named exactly the same as its enclosing namespace
	// (Duplex::Log) makes every qualified static call (Log::Renderer()) ambiguous between the
	// namespace and the class for MSVC/clang alike (DUPLEX_NS_WINDOW::Window sidesteps this
	// today only because every Window member is called through an instance, never Window::Foo()).
	class Logger
	{
	public:
		static void Init();
		static void Shutdown();

		static spdlog::logger& Core();
		static spdlog::logger& Renderer();
		static spdlog::logger& Physics();
		static spdlog::logger& Audio();
		static spdlog::logger& ECS();
		static spdlog::logger& Editor();
		static spdlog::logger& Window();
	};
}
