#pragma once
#include "TrajectoryMgr.h"
#include "CommData.h"

class CParameters
{
public:
	CParameters();
	~CParameters();

	void EnableParameters(bool enable);
	void UpdateWithReceivedData(SParameters parameters);

private:
	void SendReadParamRequest();
	void WriteParams(bool storeToFlash);

private:
	bool m_Enabled;

	// Parameters
	SParameters m_LocalParameters;
};