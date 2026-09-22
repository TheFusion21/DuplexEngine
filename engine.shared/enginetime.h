#pragma once

// EXTERNAL INCLUDES
#include <chrono>
// INTERNAL INCLUDES
#include "math/types.h"
#include "namespaces.h"

namespace DUPLEX_NS_UTIL
{
	enum class TimeType {
		miliseconds = 1,
		seconds = 1000,
		minutes = 60000,
		hours = 3600000
	};
	class Timer
	{
	private:
		bool m_Started;
		std::chrono::time_point<std::chrono::system_clock> StartTimePoint;

	public:
		Timer()
			: m_Started(false) {}

		/// <summary>
		/// Starts the timer
		/// </summary>
		void Start();
		/// <summary>
		/// Stops and resets timer
		/// </summary>
		void Stop();
		/// <summary>
		/// Get the duration the timer is running for
		/// </summary>
		/// <param name="t">Type in which the duration should be converted in</param>
		/// <returns>Time value</returns>
		real GetDuration(TimeType t);
		/// <summary>
		/// Check if the timer is running
		/// </summary>
		/// <returns></returns>
		inline bool IsStarted() const { return m_Started; }
	};
	class Time
	{
	private:
		Time() {}
		static real _deltaTime;
		static real _unscaledDeltaTime;
		static real _time;
		static real _unscaledTime;
		static real _sinTime;
		static Timer deltaTimer;
	public:
		/// <summary>
		/// Read only deltaTime for counter or animation or shit
		/// </summary>
		static const real& deltaTime;
		static const real& unscaledDeltaTime;
		static const real& time;
		static const real& unscaledTime;
		static const real& sinTime;
		static real maxDeltaTime;
		static real fixedDeltaTime;
		static real timeScale;
		/// <summary>
		/// Start time measurement
		/// </summary>
		static void Start();
		/// <summary>
		/// Update time measurements
		/// </summary>
		static void Update();

	};
}