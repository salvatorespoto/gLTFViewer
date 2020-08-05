#include "Timer.h"
#include "Windows.h"

Timer::Timer() : m_startTime(0), m_currentTime(0), m_lastTime(0), m_deltaTime(0.0), m_totalTime(0.0), m_stopped(true),
m_stopTime(0.0), m_totalStopTime(0)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&m_countsForSecond);
	m_secondsForCount = 1.0f / m_countsForSecond; // Inverse frequency
}

void Timer::reset()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_currentTime);
	m_startTime = m_currentTime;
	m_lastTime = 0;
	m_deltaTime = m_totalTime = m_totalStopTime = 0.0;
}

void Timer::stop()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_stopTime);
	m_stopped = true;
}

void Timer::start()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_currentTime);
	m_totalStopTime += (m_stopTime - m_currentTime) * m_secondsForCount;
	m_stopTime = 0.0;
	m_stopped = false;
}

double Timer::tick()
{
	if (m_stopped) return 0.0;
	QueryPerformanceCounter((LARGE_INTEGER*)&m_currentTime);
	m_deltaTime = static_cast<double>(m_currentTime - m_lastTime) * m_secondsForCount;
	m_totalTime = static_cast<double>(m_currentTime - m_startTime) * m_secondsForCount;
	m_lastTime = m_currentTime;
	return m_deltaTime;
}

double Timer::total()
{
	return (m_totalTime - m_stopTime) * m_secondsForCount;
}