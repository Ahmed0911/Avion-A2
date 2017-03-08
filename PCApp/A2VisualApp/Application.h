#pragma once
#include <fstream>
#include "Serial.h"
#include "EthernetComm.h"
#include "SerialComm.h"
#include "CommMgr.h"
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
	void NewPacketReceived(BYTE type, BYTE* data, int len);
	
	// commands
	void DownloadWaypoints(SWaypoint* wps, int cnt);
	void ExecuteTrajectory(float velocity);
	void ExecuteTarget(SWaypoint target);
	void ExecuteOrbit(SWaypoint target, float velocity);
	CDir2D m_dir2D;

private:
	void FillHopeRFData(SUserData& drawData);
	void FillEthernetData(SUserData& drawData);
	void GenerateLogBitmaps(std::wstring logFilename, std::wstring destination);
	void GenerateLogBitmapsHopeRF(std::wstring logFilename, std::wstring destination);
	void GenerateLogFile(std::wstring logFilename, std::wstring destination);

private:
	static CApplication* instance;

private:
	// Communication
	CEthernetComm m_ethernetCommA2;
	CCommMgr m_commMgr;
	CSerial m_SerialGPS;
	
	SCommEthData m_RXGatewayData;
	SCommEthData m_RXEthData;
	SCommHopeRFDataA2Avion m_RXHopeRFData;
	// counters
	int m_RXRSSI;
	int m_RXHopeRFPacketCounter;
	int m_RXHopeRFCRCErrorCounter;

	// no telemetry stuff
	bool m_NoTelemetry;
	double m_lastTelemetryReceivedTimestamp;

	// GPS
	UBloxGPS m_GPS;

	// perf + log
	int m_TimerCounter;
	std::ofstream m_LogFileDraw;
	std::ofstream m_LogFileHopeRF;

	// filters
	CAvgFilter m_FilterControlStationRSSI;
	CAvgFilter m_FilterA2RSSI;
	CAvgFilter m_FilterVertSpeed;
	CAvgFilter m_FilterSpeed;
	CAvgFilter m_FilterFuelPercent;


	// Commands
	unsigned char m_ModeRequest;
};
