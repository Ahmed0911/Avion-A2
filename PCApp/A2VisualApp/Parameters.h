#pragma once
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_2.h>
#include <D2d1helper.h>
#include <D2d1_1helper.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <Dwrite.h>
#include "DirectXHelper.h"
#include <wrl\client.h>
using namespace Microsoft::WRL;
#include <functional>
#include "TrajectoryMgr.h"
#include "CommData.h"
#include <vector>

class CParameters
{
public:
	CParameters();
	~CParameters();

	void Init(ID2D1DeviceContext* d2dDeviceContext, IDWriteFactory* writeFactory, HWND hWnd);
	void UpdateWithReceivedData(SParameters parameters);
	void Draw(ID2D1DeviceContext* d2dDeviceContext);
	void Keydown(WPARAM wParam);

private:
	void EnableParameters(bool enable);
	void SendReadParamRequest();
	void WriteParams(bool storeToFlash);

private:
	bool m_Enabled;

	// D3D Stuff
	ComPtr<ID2D1SolidColorBrush> m_DrawBrush;
	ComPtr<ID2D1SolidColorBrush> m_DrawBrushRed;
	ComPtr<IDWriteTextFormat> m_DrawTextFormat;

	// Parameters
	SParameters m_LocalParameters;

	// Selection
	int m_Selected;

	// Structs
	struct SParamDrawData
	{
		SParamDrawData(std::wstring name, float min, float max, float step, float* dataPtr)
		{
			Name = name;
			Min = min;
			Max = max;
			Step = step;
			DataPtr = dataPtr;
		}

		std::wstring Name;		
		float Min;
		float Max;
		float Step;
		float* DataPtr;
	};

	std::vector<SParamDrawData> m_ParamsDrawArray;

};