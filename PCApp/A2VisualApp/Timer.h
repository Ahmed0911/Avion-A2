#pragma once

class CPerformanceTimer
{
private:
	LARGE_INTEGER m_frequency;
	LARGE_INTEGER m_LastTime;

public:
	CPerformanceTimer(void);
	~CPerformanceTimer(void);

	void StartTheClock(void);
	double GetDelta(void);
	double GetDeltaAndReset(void);
	static double GetCurrentTimestamp(void);
};
