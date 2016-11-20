#include "stdAfx.h"
#include "Timer.h"

CPerformanceTimer::CPerformanceTimer(void)
{
	QueryPerformanceFrequency(&m_frequency);
}

CPerformanceTimer::~CPerformanceTimer(void)
{
}

void CPerformanceTimer::StartTheClock(void)
{
	 QueryPerformanceCounter(&m_LastTime);
}

double CPerformanceTimer::GetDelta(void)
{
	LARGE_INTEGER newTime;
	QueryPerformanceCounter(&newTime);

	return (double)((newTime.QuadPart - m_LastTime.QuadPart) / (double)m_frequency.QuadPart);
}

double CPerformanceTimer::GetDeltaAndReset(void)
{
	double delta = GetDelta();
	StartTheClock();

	return delta;
}

double CPerformanceTimer::GetCurrentTimestamp()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	return ((double)currentTime.QuadPart / (double)frequency.QuadPart);
}