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
#include <wincodec.h>
#include "TrajectoryMgr.h"

class CMapManager
{
public:
	CMapManager();
	~CMapManager();

	void LoadMap(TCHAR* mapname, ID2D1DeviceContext* d2dDeviceContext, IDWriteFactory* writeFactory, HWND hWnd);
	void ConvertLocationLL2Pix(double longitude, double latitude, float& mapX, float& mapY);
	void ConvertLocationPix2LL(double& longitude, double& latitude, float mapX, float mapY);
	void Draw(ID2D1DeviceContext* d2dDeviceContext, double longitude, double latitude);

	void SetHome(double longitude, double latitude, double heightMSL);
	double DistanceFromHomeMeters(double longitude, double latitude);
	bool HaveHome();

	void SetMapWpyCommands(WPARAM param);
	void SetMapWpyClick(int xPos, int yPos);
	int m_ActiveWaypoint;

private:
	double LLDistanceM(double longitude1, double latitude1, double longitude2, double latitude2);

private:
	HWND m_hWnd;
	double m_MetersPerPix;
	double m_ZoomLevel;
	POINT m_MapImageSizePix;
	double m_CenterLongitude;
	double m_CenterLatitude;
	int m_SelectedWaypoint;

	// home position
	bool m_HomeSet;
	double m_HomeLongitude;
	double m_HomeLatitude;
	double m_HomeHeightMSL; // baro pressure should be zeroed on homing

	CTrajectoryMgr m_trajMgr;	
	float m_SelectedAltitude;
	float m_SelectedVelocity;

	SWaypoint m_Target;

	// brushes
	ComPtr<ID2D1SolidColorBrush> m_PointBrush;
	ComPtr<ID2D1SolidColorBrush> m_CircleBrush;
	ComPtr<ID2D1SolidColorBrush> m_HomeBrush;
	ComPtr<ID2D1SolidColorBrush> m_WaypointBrush;
	ComPtr<IDWriteTextFormat> m_HomeTextFormat;
	ComPtr<IDWriteTextFormat> m_WaypointFormat;
	float m_CircleSize;

	// WIC
	ComPtr<IWICImagingFactory2> m_wicFactory;
	ComPtr<ID2D1Bitmap1> m_mapBitmap;
};

// links:
// http://dev.virtualearth.net/REST/v1/Imagery/Map/Road/47.619048,-122.35384/15?mapSize=500,500&pp=47.620495,-122.34931;21;AA&pp=47.619385,-122.351485;;AB&pp=47.616295,-122.3556;22&mapMetadata=1&o=xml&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
// http://dev.virtualearth.net/REST/v1/Imagery/Map/Road/45.787469,15.889805/15?mapSize=800,600&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
// max size: http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/45.787469,15.889805/6?mapSize=1440,937&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
// MAP, JPG: http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/45.787469,15.889805/16?mapSize=1000,800&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
// METADATA: http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/45.787469,15.889805/16?mapSize=1000,800&o=xml&mapMetadata=1&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
