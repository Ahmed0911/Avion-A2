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
	m_Selected = 0;

	// Fill parameters array
	m_ParamsDrawArray.push_back(SParamDrawData(L"Roll P", 0, 0.1f, 0.005f, &m_LocalParameters.RollKp));
	m_ParamsDrawArray.push_back(SParamDrawData(L"Roll I", 0, 0.02f, 0.001f, &m_LocalParameters.RollKi));
	m_ParamsDrawArray.push_back(SParamDrawData(L"Roll D", 0, 0.02f, 0.001f, &m_LocalParameters.RollKd));
	m_ParamsDrawArray.push_back(SParamDrawData(L"Pitch P", 0, 0.1f, 0.005f, &m_LocalParameters.PitchKp));
	m_ParamsDrawArray.push_back(SParamDrawData(L"Pitch I", 0, 0.02f, 0.001f, &m_LocalParameters.PitchKi));
	m_ParamsDrawArray.push_back(SParamDrawData(L"Pitch D", 0, 0.02f, 0.001f, &m_LocalParameters.PitchKd));
}


CParameters::~CParameters()
{
}

void CParameters::Init(ID2D1DeviceContext* d2dDeviceContext, IDWriteFactory* writeFactory, HWND hWnd)
{
	// Create Brushes
	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow), &m_DrawBrush)
	);

	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &m_DrawBrushRed)
	);
	

	// Create Text Font
	DX::ThrowIfFailed(
		writeFactory->CreateTextFormat(
			L"Arial",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			24,
			L"", //locale
			&m_DrawTextFormat)
	);
	m_DrawTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_DrawTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void CParameters::Draw(ID2D1DeviceContext* d2dDeviceContext)
{	
	if (!m_Enabled) return;
	
	// Draw Parameters array
	for (int i = 0; i != m_ParamsDrawArray.size(); i++)
	{
		SParamDrawData p = m_ParamsDrawArray[i];
		
		float X = 300;
		float Y = -400+ i * 50.0f;

		TCHAR numberStr[100];
		// Name
		swprintf_s(numberStr, 100, L"%s", p.Name.c_str());
		d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_DrawTextFormat.Get(), &D2D1::RectF(X, Y, X+100, Y+50), m_DrawBrush.Get());

		// Min/Max
		//swprintf_s(numberStr, 100, L"%0.2f", p.Min);
		//d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_DrawTextFormat.Get(), &D2D1::RectF(X+80, Y, X+100 + 80, Y + 50), m_DrawBrush.Get());
		//swprintf_s(numberStr, 100, L"%0.2f", p.Max);
		//d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_DrawTextFormat.Get(), &D2D1::RectF(X + 300, Y, X + 300 + 100, Y + 50), m_DrawBrush.Get());
		// Value
		swprintf_s(numberStr, 100, L"%0.3f", *p.DataPtr);
		d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_DrawTextFormat.Get(), &D2D1::RectF(X + 200, Y, X + 200 + 100, Y + 50), m_DrawBrush.Get());

		if (m_Selected == i)
		{
			// Draw Selection
			d2dDeviceContext->DrawRectangle(D2D1::RectF(X, Y, X+400, Y+50), m_DrawBrush.Get(), 2);
			d2dDeviceContext->DrawLine(D2D1::Point2F(X + 100, Y), D2D1::Point2F(X + 100, Y + 50), m_DrawBrush.Get());

			// Draw Value
			float Value = (*p.DataPtr - p.Min) / (p.Max - p.Min) * 300;
			d2dDeviceContext->DrawLine(D2D1::Point2F(X + 100+ Value, Y), D2D1::Point2F(X + 100+Value, Y + 50), m_DrawBrushRed.Get(), 2);
		}
	}


}

void CParameters::Keydown(WPARAM wParam)
{
	switch (wParam)
	{
		case 0x50: // 'P'
			EnableParameters(true);
			break;

		case 0x4F: // 'O'
			EnableParameters(false);
			break;

		case 0x57: // 'W'
			WriteParams(true); // write to flash!!!
			break;

		case VK_UP:
			if (--m_Selected < 0) m_Selected = 0;
			break;

		case VK_DOWN:
			if (++m_Selected >= (int)m_ParamsDrawArray.size()) m_Selected = m_ParamsDrawArray.size() - 1;
			break;

		case VK_LEFT:
		{
			// step param LEFT
			SParamDrawData p = m_ParamsDrawArray[m_Selected];
			*p.DataPtr -= p.Step;
			if (*p.DataPtr < p.Min) *p.DataPtr = p.Min;

			// write params
			WriteParams(false);

			break;
		}

		case VK_RIGHT:
		{
			// step param RIGHT
			SParamDrawData p = m_ParamsDrawArray[m_Selected];
			*p.DataPtr += p.Step;
			if (*p.DataPtr > p.Max) *p.DataPtr = p.Max;

			// write params
			WriteParams(false);

			break;
		}
	}
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
