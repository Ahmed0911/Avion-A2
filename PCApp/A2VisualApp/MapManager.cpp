#include "stdafx.h"
#include "MapManager.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>
#include "Application.h"
#include <vector>
#include <string>
using namespace std;

extern CApplication app;

vector<wstring> split(const TCHAR *str, TCHAR c = ' ')
{
	vector<wstring> result;

	do
	{
		const TCHAR *begin = str;
		while (*str != c && *str) str++;

		result.push_back(wstring(begin, str));
	} while (0 != *str++);

	return result;
}

CMapManager::CMapManager()
{	
	m_CircleSize = 0;
	m_HomeSet = false;
	m_SelectedWaypoint = 0;
	m_SelectedAltitude = 10;
	m_SelectedVelocity = 3;
	m_Target.Latitude = 0; // inactive
	m_ActiveWaypoint = -1;
}


CMapManager::~CMapManager()
{
}



void CMapManager::LoadMap(TCHAR* mapname, ID2D1DeviceContext* d2dDeviceContext, IDWriteFactory* writeFactory, HWND hWnd)
{
	m_hWnd = hWnd;

	// create brushes
	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &m_PointBrush)
		);

	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::AliceBlue), &m_CircleBrush)
		);

	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &m_WaypointBrush)
		);

	DX::ThrowIfFailed(
		d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_HomeBrush)
		);

	// Create "HOME" font
	DX::ThrowIfFailed(
		writeFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24,
		L"", //locale
		&m_HomeTextFormat)
		);
	m_HomeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_HomeTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	DX::ThrowIfFailed(
		writeFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		14,
		L"", //locale
		&m_WaypointFormat)
		);
	m_WaypointFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_WaypointFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// LOAD MAP
	// create WiC Encoder
	DX::ThrowIfFailed(
		CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_wicFactory))
	);

	ComPtr<IWICBitmapDecoder> wicBitmapDecoder;
	DX::ThrowIfFailed(
		m_wicFactory->CreateDecoderFromFilename(
			mapname,
			NULL,
			GENERIC_READ,
			WICDecodeMetadataCacheOnLoad,
			&wicBitmapDecoder)
	);

	// Create the initial frame.
	ComPtr<IWICBitmapFrameDecode> pSource;
	DX::ThrowIfFailed(
		wicBitmapDecoder->GetFrame(0, &pSource)
	);

	// Convert the image format to 32bppPBGRA
	// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	ComPtr<IWICFormatConverter> pConverter;
	DX::ThrowIfFailed(
		m_wicFactory->CreateFormatConverter(&pConverter)
	);

	DX::ThrowIfFailed(
		pConverter->Initialize(
			pSource.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut )
	);

	// Create a Direct2D bitmap from the WIC bitmap.
	DX::ThrowIfFailed(
		d2dDeviceContext->CreateBitmapFromWicBitmap(
			pConverter.Get(),
			NULL,
			&m_mapBitmap)
	);

	// get map data
	vector<wstring> toks = split(mapname, '-');
	m_CenterLatitude = _tstof(toks[1].c_str());
	m_CenterLongitude = _tstof(toks[2].c_str());
	m_ZoomLevel = _tstof(toks[3].c_str());
	
	UINT sizeX, sizeY;
	pSource->GetSize(&sizeX, &sizeY);
	m_MapImageSizePix.x = sizeX;
	m_MapImageSizePix.y = sizeY;
	
	m_MetersPerPix = 156543.04 * cos(m_CenterLatitude / 180 * M_PI) / (pow(2, m_ZoomLevel));
}

void CMapManager::Draw(ID2D1DeviceContext* d2dDeviceContext, double longitude, double latitude)
{
	// draw map
	d2dDeviceContext->DrawBitmap(m_mapBitmap.Get(), &D2D1::RectF(0, 0, (float)m_MapImageSizePix.x, (float)m_MapImageSizePix.y), 1, D2D1_INTERPOLATION_MODE_ANISOTROPIC);

	// draw location
	float mapX, mapY;
	ConvertLocationLL2Pix(longitude, latitude, mapX, mapY);
	// clip to map!
	if (mapX < 0) mapX = 0;
	if (mapY < 0) mapY = 0;
	if (mapX > m_MapImageSizePix.x) mapX = (float)m_MapImageSizePix.x;
	if (mapY > m_MapImageSizePix.y) mapY = (float)m_MapImageSizePix.y;
	d2dDeviceContext->FillEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 10, 10), m_PointBrush.Get());
	d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 10 + m_CircleSize, 10 + m_CircleSize), m_CircleBrush.Get(), 5);
	if (++m_CircleSize > 50) m_CircleSize = 0;

	// draw home
	if (m_HomeSet)
	{
		ConvertLocationLL2Pix(m_HomeLongitude, m_HomeLatitude, mapX, mapY);
		d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 16, 16), m_HomeBrush.Get(), 3);		
		d2dDeviceContext->DrawText(L"H", 1, m_HomeTextFormat.Get(), &D2D1::RectF(mapX - 10, mapY - 10, mapX + 10, mapY + 10), m_HomeBrush.Get());	

		// Draw Target Circles
		float DistanceM = 500;
		//d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), DistanceM / m_MetersPerPix, DistanceM / m_MetersPerPix), m_PointBrush.Get(), 2);
	}

	if (m_SelectedWaypoint == 0)
	{
		// immediate mode
		if (m_Target.Latitude != 0)
		{
			ConvertLocationLL2Pix(m_Target.Longitude, m_Target.Latitude, mapX, mapY);
			d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 16, 16), m_HomeBrush.Get(), 3);	
			d2dDeviceContext->DrawLine(D2D1::Point2F(mapX - 20, mapY - 20), D2D1::Point2F(mapX + 20, mapY + 20), m_HomeBrush.Get(), 1, NULL);
			d2dDeviceContext->DrawLine(D2D1::Point2F(mapX + 20, mapY - 20), D2D1::Point2F(mapX - 20, mapY + 20), m_HomeBrush.Get(), 1, NULL);
			TCHAR numberStr[100];
			swprintf_s(numberStr, 100, L"%d", (int)m_Target.Altitude);
			d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_WaypointFormat.Get(), &D2D1::RectF(mapX - 20, mapY - 10, mapX + 20, mapY + 70), m_WaypointBrush.Get());
		}
	}
	else if(m_SelectedWaypoint == 9)
	{
		// orbit mode
		if (m_Target.Latitude != 0)
		{
			ConvertLocationLL2Pix(m_Target.Longitude, m_Target.Latitude, mapX, mapY);
			d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 32, 32), m_CircleBrush.Get(), 5);
			d2dDeviceContext->DrawLine(D2D1::Point2F(mapX - 20, mapY - 20), D2D1::Point2F(mapX + 20, mapY + 20), m_HomeBrush.Get(), 1, NULL);
			d2dDeviceContext->DrawLine(D2D1::Point2F(mapX + 20, mapY - 20), D2D1::Point2F(mapX - 20, mapY + 20), m_HomeBrush.Get(), 1, NULL);
			TCHAR numberStr[100];
			swprintf_s(numberStr, 100, L"%d", (int)m_Target.Altitude);
			d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_WaypointFormat.Get(), &D2D1::RectF(mapX - 20, mapY - 10, mapX + 20, mapY + 70), m_WaypointBrush.Get());
		}
	}
	else
	{
		// draw trajectory
		SWaypoint wps[10];
		int cnt = m_trajMgr.GetWaypoints(wps);
		float lastWpX, lastWpY;
		for (int i = 0; i != cnt; i++)
		{
			ConvertLocationLL2Pix(wps[i].Longitude, wps[i].Latitude, mapX, mapY);
			d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 16, 16), m_WaypointBrush.Get(), 3);
			TCHAR numberStr[100];
			swprintf_s(numberStr, 100, L"%d", i + 1);
			d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_HomeTextFormat.Get(), &D2D1::RectF(mapX - 10, mapY - 10, mapX + 10, mapY + 10), m_WaypointBrush.Get());
			swprintf_s(numberStr, 100, L"%d", (int)wps[i].Altitude);
			d2dDeviceContext->DrawText(numberStr, (UINT32)_tcslen(numberStr), m_WaypointFormat.Get(), &D2D1::RectF(mapX - 20, mapY - 10, mapX + 20, mapY + 70), m_WaypointBrush.Get());

			// draw selected waypoint
			if (i == m_ActiveWaypoint)
			{
				d2dDeviceContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(mapX, mapY), 20, 20), m_HomeBrush.Get(), 3);
			}

			// draw line
			if (i > 0) d2dDeviceContext->DrawLine(D2D1::Point2F(lastWpX, lastWpY), D2D1::Point2F(mapX, mapY), m_WaypointBrush.Get(), 1, NULL);
			lastWpX = mapX;
			lastWpY = mapY;
		}
	}
	// draw data
	TCHAR str[100];
	if (m_SelectedWaypoint == 0) swprintf_s(str, 100, L"Immediate");
	else if (m_SelectedWaypoint == 9) swprintf_s(str, 100, L"Orbit");
	else swprintf_s(str, 100, L"Index: %d", m_SelectedWaypoint);
	//d2dDeviceContext->DrawText(str, (UINT32)_tcslen(str), m_HomeTextFormat.Get(), &D2D1::RectF(200, 550, 350, 600), m_HomeBrush.Get());
	swprintf_s(str, 100, L"Altitude: %d", (int)m_SelectedAltitude);
	//d2dDeviceContext->DrawText(str, (UINT32)_tcslen(str), m_HomeTextFormat.Get(), &D2D1::RectF(350, 550, 500, 600), m_HomeBrush.Get());
	swprintf_s(str, 100, L"Velocity: %0.1f", m_SelectedVelocity);
	//d2dDeviceContext->DrawText(str, (UINT32)_tcslen(str), m_HomeTextFormat.Get(), &D2D1::RectF(500, 550, 650, 600), m_HomeBrush.Get());
}

void CMapManager::SetMapWpyCommands(WPARAM param)
{
	if (param >= '0' && param <= '0' + MAXWAYPOINTS+1)
	{
		int index = param - '0';
		m_SelectedWaypoint = index;
	}
	
	// Altitude
	if (param == VK_OEM_PLUS || param == VK_ADD) m_SelectedAltitude += 2;
	if (param == VK_OEM_MINUS || param == VK_SUBTRACT) m_SelectedAltitude -= 2;
	if (m_SelectedAltitude < 0) m_SelectedAltitude = 0;

	// Velocity
	if (param == VK_UP ) m_SelectedVelocity += 0.5;
	if (param == VK_DOWN) m_SelectedVelocity -= 0.5;
	if (m_SelectedVelocity < 0.5) m_SelectedVelocity = 0.5;
	if (m_SelectedVelocity > 5) m_SelectedVelocity = 5;

	// delete waypoints
	if (param == VK_DELETE) m_trajMgr.DeleteWaypoint();

	// save/load
	if (param == 'S') m_trajMgr.SaveWaypoints(); // save
	if (param == 'L') m_trajMgr.LoadWaypoints(); // load

	if (param == VK_SPACE)
	{
		// download waypoints
		SWaypoint wps[10];
		int cnt = m_trajMgr.GetWaypoints(wps);
		app.DownloadWaypoints(wps, cnt);
	}
	if (param == VK_RETURN)
	{
		app.ExecuteTrajectory(m_SelectedVelocity);
	}
}

void CMapManager::SetMapWpyClick(int xPos, int yPos)
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);

	// X=288-1537
	// Y=53-1054
	float mapX = ((xPos / (float)(rect.right - rect.left) * 1920) - 288) / (1537 - 288)*m_MapImageSizePix.x;
	float mapY = ((yPos / (float)(rect.bottom - rect.top) * 1080) - 53) / (1054 - 53)*m_MapImageSizePix.y;

	double lon, lat;
	ConvertLocationPix2LL(lon, lat, mapX, mapY);

	if (m_SelectedWaypoint == 9)
	{
		m_Target.Altitude = m_SelectedAltitude;
		m_Target.Latitude = lat;
		m_Target.Longitude = lon;

		app.ExecuteOrbit(m_Target, m_SelectedVelocity);
	}
	else if (m_SelectedWaypoint > 0 )
	{
		m_trajMgr.AddWaypoint(m_SelectedWaypoint - 1, m_SelectedAltitude, lat, lon);
		m_SelectedWaypoint++;
		if (m_SelectedWaypoint > MAXWAYPOINTS) m_SelectedWaypoint = MAXWAYPOINTS;
	}
	else
	{
		m_Target.Altitude = m_SelectedAltitude;
		m_Target.Latitude = lat;
		m_Target.Longitude = lon;

		/// send target
		app.ExecuteTarget(m_Target);
	}
}

void CMapManager::ConvertLocationLL2Pix(double longitude, double latitude, float& mapX, float& mapY)
{
	double deltaX = longitude - m_CenterLongitude;
	double pixelX = (deltaX / 360) * 256 * pow(2, m_ZoomLevel);
	mapX = (float)(pixelX + m_MapImageSizePix.x/2);

	double sinLatitudeCenter = sin(m_CenterLatitude * M_PI / 180);
	double pixelYCenter = (0.5 - log((1 + sinLatitudeCenter) / (1 - sinLatitudeCenter)) / (4 * M_PI)) * 256 * pow(2, m_ZoomLevel); // center pix
	double sinLatitude = sin(latitude * M_PI / 180);
	double pixelY = (0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI)) * 256 * pow(2, m_ZoomLevel);
	mapY = float(pixelY - pixelYCenter + m_MapImageSizePix.y/2);
}

void CMapManager::ConvertLocationPix2LL(double& longitude, double& latitude, float mapX, float mapY)
{
	double pixelX = mapX - m_MapImageSizePix.x / 2;
	double deltaX = pixelX * 360 / (256 * pow(2, m_ZoomLevel));
	longitude = deltaX + m_CenterLongitude;

	double sinLatitudeCenter = sin(m_CenterLatitude * M_PI / 180);
	double pixelYCenter = (0.5 - log((1 + sinLatitudeCenter) / (1 - sinLatitudeCenter)) / (4 * M_PI)) * 256 * pow(2, m_ZoomLevel); // center pix
	double pixelY = mapY + pixelYCenter - m_MapImageSizePix.y / 2;
	double deltaY = 0.5 - pixelY / (256 * pow(2, m_ZoomLevel));
	latitude = 90 - 360 * atan(exp(-deltaY * 2 * M_PI)) / M_PI;
}

void CMapManager::SetHome(double longitude, double latitude, double heightMSL)
{
	m_HomeLongitude = longitude;
	m_HomeLatitude = latitude;
	m_HomeHeightMSL = heightMSL;
	m_HomeSet = true;
}

bool CMapManager::HaveHome()
{
	return m_HomeSet;
}

double CMapManager::DistanceFromHomeMeters(double longitude, double latitude)
{
	double distance = 0; // no home set
	if (m_HomeSet)
	{
		distance = LLDistanceM(longitude, latitude, m_HomeLongitude, m_HomeLatitude);
	}
	
	return distance;
}

double CMapManager::LLDistanceM(double longitude1, double latitude1, double longitude2, double latitude2)
{
	double R = 6371000; // [m]
	double dLat = (latitude2 - latitude1) * M_PI / 180;
	double dLon = (longitude2 - longitude1) * M_PI / 180;
	double lat1 = latitude1 * M_PI / 180;
	double lat2 = latitude2 * M_PI / 180;

	double a = sin(dLat / 2) * sin(dLat / 2) + sin(dLon / 2) * sin(dLon / 2) * cos(lat1) * cos(lat2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));
	double distance = R * c;

	return distance;
}