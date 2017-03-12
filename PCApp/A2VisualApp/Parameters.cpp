#include "stdafx.h"
#include "Parameters.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>
#include "Application.h"
#include <vector>
#include <string>
using namespace std;

CParameters::CParameters()
{
	m_Enabled = false;
}


CParameters::~CParameters()
{
}

void CParameters::UpdateWithReceivedData(SParameters parameters)
{
	m_LocalParameters = parameters;
}

void CParameters::EnableParameters(bool enable)
{
	if (enable == true)
	{
		// enable params and send read request (fill latest params data)
		m_Enabled = true;
		SendReadParamRequest();
	}
	else
	{
		// disable params display
		m_Enabled = false;
	}
}

void CParameters::SendReadParamRequest()
{
	CApplication* app = CApplication::getInstance();

	app->ParamsReadCmd();
}

void CParameters::WriteParams(bool storeToFlash)
{
	CApplication* app = CApplication::getInstance();

	// send write request
	app->ParamsWrite(m_LocalParameters);

	if (storeToFlash)
	{
		// store params to flash
		app->ParamsStoreToFlash();
	}
}
