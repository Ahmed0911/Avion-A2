#pragma once

#define AVGFILTER_MAXSIZE 1024

class CAvgFilter
{
public:
	void Init(int size);
	float Add(float data);
	float GetAverage();

private:
	float m_Buffer[AVGFILTER_MAXSIZE];
	float m_TotalSum;
	int m_Size;
	int m_Index;
};

