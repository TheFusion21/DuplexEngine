#include "Time.h"

using namespace Engine::Utils;

void Timer::Start()
{
	m_Started = true;
	StartTimePoint = std::chrono::system_clock::now();
}

void Timer::Stop()
{
	m_Started = false;
	StartTimePoint = std::chrono::time_point<std::chrono::system_clock>::min();
}

real Timer::GetDuration(TimeType t)
{
	std::chrono::duration<real> dur = (std::chrono::system_clock::now() - StartTimePoint);
	real mils = dur.count() * static_cast<real>(1000.0);
	return mils / static_cast<real>(t);
}

Timer Time::deltaTimer;
real Time::_deltaTime = 0;
real Time::_time = 0;
real Time::_sinTime = 0;
const real& Time::time = Time::_time;
const real& Time::deltaTime = Time::_deltaTime;
const real& Time::sinTime = Time::_sinTime;

void Time::Start()
{
	deltaTimer.Start();
}

void Time::Update()
{
	_deltaTime = deltaTimer.GetDuration(TimeType::seconds);
	_time += _deltaTime;
#ifdef DOUBLEPRECISION
	_sinTime = static_cast<real>(sin(_time));
#else
	_sinTime = static_cast<real>(sinf(_time));
#endif
	deltaTimer.Start();
}


