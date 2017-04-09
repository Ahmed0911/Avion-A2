#pragma once

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_2.h>
#include <D2d1helper.h>
#include <D2d1_1helper.h>
#include <Dwrite.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include "DirectXHelper.h"
#include <wrl\client.h>
using namespace Microsoft::WRL;
#include <functional>
#include <wincodec.h>
#include "CamGrabber.h"
#include "MapManager.h"
#include "Parameters.h"

struct SUserData
{
	unsigned int LoopCounter;	

	// IMU
	float Roll;		// [deg]
	float Pitch;	// [deg]
	float Yaw;		// [deg]
	float dRoll;	// [deg/s]
	float dPitch;	// [deg/s]
	float dYaw;		// [deg/s]

	// alt/speed
	float Pressure; // [Pa]	
	float Altitude; // [m]
	float Vertspeed;// [m/s]
	float Speed;	// [m/s]

	// engines
	unsigned char MotorThrusts[4]; // [0-100%]

	// Fuel
	float BatteryVoltage; // [V]
	float BatteryCurrentA; // [A]
	float BatteryTotalCharge_mAh; // [mAh]
	float FuelLevel; // [0...100%]

	// GPS
	unsigned int GPSTime;
	unsigned char FixType;
	unsigned char FixFlags; // FIX Flags
	unsigned char NumSV;
	unsigned char ActualMode; // Non GPS, but used for 32bit alignment
	int Longitude; // 1e-7 [deg]
	int Latitude; // 1e-7 [deg]
	int HeightMSL; // MSL [mm]
	unsigned int HorizontalAccuracy; // [mm]
	unsigned int VerticalAccuracy; // [mm]
	int VelN; // Speed North [mm/s]
	int VelE; // Speed East [mm/s]
	int VelD; // Speed Down [mm/s]
	unsigned int SpeedAcc; // Speed accuracy [mm/s]

	// SDCard
    unsigned int SDCardBytesWritten;
    unsigned int SDCardFails;
    unsigned int FailedQueues;

	// Comm
	unsigned int RXA2RSSIFrameCount;
	unsigned int RXControlStationFrameCount;
	int RXA2RSSI;
	int RXControlStationRSSI;

	// Comm433MHz/CommMgr
	int MsgReceivedOK;
	int CrcErrors;
	int HeaderFails;
	int TimeoutCounter;

	double LocalTime;
};

#define DNSTYLE_SMALL 0
#define DNSTYLE_YAW_BOX 1
#define DNSTYLE_YAW_BOX_FLOAT 2
#define DNSTYLE_RED_WARNING 3

class CDir2D
{
public:
	CDir2D(void);
	~CDir2D(void);

	void Init(HWND hWnd, TCHAR* mapName);
	void Shutdown(void);
	void Draw(SUserData& data, bool noTelemetry);
	void DrawToBitmap(SUserData &data, std::wstring bitmapfile, bool noTelemetry);
	void SetMapWpyCommands(WPARAM param);
	CMapManager* GetMapMgr();

public:
	enum EActiveDisplay { CAMERA, MAP };
	EActiveDisplay m_ActiveDisplay;

	// Parameters
	CParameters m_Parameters;

private:
	void CreateUserResources(void);
	void DrawHUD(SUserData &data, bool noTelemetry);
	void DrawVideo(void);
	void DrawNumber(int style, float x, float y, float dx, float dy, int number, float numberF = 0, TCHAR* text = NULL);

private:
	ComPtr<ID2D1Factory1>	m_d2dFactory;
    D3D_FEATURE_LEVEL		m_featureLevel;
	ComPtr<ID2D1Device>		m_d2dDevice;
	ComPtr<ID2D1DeviceContext> m_d2dContext;
	ComPtr<IDXGISwapChain1> m_swapChain;
	ComPtr<ID2D1Bitmap1>	m_d2dTargetBitmap;
	float m_dpi;

	// resources
	ComPtr<IDWriteFactory> m_DWriteFactory;
	
	ComPtr<ID2D1SolidColorBrush> m_GreenBrush;
	ComPtr<ID2D1SolidColorBrush> m_RedBrush;
	ComPtr<ID2D1SolidColorBrush> m_BlueBrush;
	ComPtr<ID2D1LinearGradientBrush>	m_pLinearGradientRYGBrush;
	ComPtr<ID2D1StrokeStyle> m_DashedLineStroke;
	ComPtr<IDWriteTextFormat> m_GreenTextFormat;
	ComPtr<IDWriteTextFormat> m_GreenLargeTextFormat;
	ComPtr<IDWriteTextFormat> m_RedLargeWaringTextFormat;
	
	// camera
	CamGrabber m_camGrabber;
	ComPtr<ID2D1Bitmap1> m_CamVideoBitmap;

	// Map
	CMapManager m_Map;

	// WIC
	ComPtr<IWICImagingFactory2> m_wicFactory;	
	ComPtr<ID2D1Bitmap1> m_saveBitmap;
};

