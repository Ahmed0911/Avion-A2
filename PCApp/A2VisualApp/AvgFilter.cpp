#include "stdafx.h"
#include "AvgFilter.h"


void CAvgFilter::Init(int size)
{
	if (size > AVGFILTER_MAXSIZE) size = AVGFILTER_MAXSIZE;
	m_Size = size;
	m_Index = 0;
	m_TotalSum = 0; 
	memset(m_Buffer, 0x00, AVGFILTER_MAXSIZE);
}

float CAvgFilter::Add(float data)
{
	// get last value
	float last = m_Buffer[m_Index];
	// remove from sum
	m_TotalSum -= last;
	// add new to sum
	m_TotalSum += data;
	// add to buffer
	m_Buffer[m_Index] = data;
	// advance index
	m_Index = (m_Index + 1) % m_Size;

	// return current aveerage
	return GetAverage();
}

float CAvgFilter::GetAverage()
{
	return m_TotalSum/m_Size;
}