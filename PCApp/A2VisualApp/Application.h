#pragma once
#include <fstream>
#include "Serial.h"
#include "XBeeComm.h"
#include "Dir2D.h"
#include "CommData.h"
#include "AvgFilter.h"
#include "UBloxGPS.h"

class CApplication
{
public:
	static CApplication* getInstance();
	void Init(HWND hWnd, TCHAR* cmdLine);
	void Shutdown();
	void OnTimer();
	void NewPacketReceived(BYTE* data, int len, int rssi, int frameType);

	void GenerateLogBitmaps(std::wstring logFilename, std::wstring destination);
	void GenerateLogFile(std::wstring logFilename, std::wstring destination);
	
	// commands
	void RequestModeChange(unsigned char newMode);
	void CheckRFSpectrum();
	void DownloadWaypoints(SWaypoint* wps, int cnt);
	void ExecuteTrajectory(float velocity);
	void ExecuteTarget(SWaypoint target);
	void ExecuteOrbit(SWaypoint target, float velocity);
	CDir2D m_dir2D;

private:
	static CApplication* instance;

private:
	CSerial m_Serial;
	CSerial m_SerialGPS;
	CXBeeComm m_XBee;
	
	SCommData m_RXData;
	int m_RXRSSI;
	int m_RXPacketCounter;

	bool m_NoTelemetry;

	// GPS
	UBloxGPS m_GPS;

	// perf + log
	int m_TimerCounter;
	std::ofstream m_LogFile;

	// filters
	CAvgFilter m_FilterControlStationRSSI;
	CAvgFilter m_FilterKikiRSSI;
	CAvgFilter m_FilterVertSpeed;
	CAvgFilter m_FilterSpeed;
	CAvgFilter m_FilterFuelPercent;


	// Commands
	unsigned char m_ModeRequest;
};
