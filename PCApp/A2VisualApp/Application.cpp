#include "stdafx.h"
#include "Application.h"

#define SERIAL_PORT L"\\\\.\\COM4"
#define SERIAL_PORT_GPS L"\\\\.\\COM6"
#include "CommData.h"
#include "Timer.h"
#include <fstream>

// Callback wrapper
void NewPacketRxWrapper(BYTE* data, int len, int rssi, int frameType)
{
	CApplication::getInstance()->NewPacketReceived(data, len, rssi, frameType);
}

CApplication* CApplication::instance;
CApplication* CApplication::getInstance( )
{
	return CApplication::instance;
}

void CApplication::Init(HWND hWnd, TCHAR* cmdLine)
{
	instance = this;
	m_TimerCounter = 0;
	m_ModeRequest = -1; // no mode set, using ctrl default
	m_NoTelemetry = false; // XXX

	// create D2D
	std::wstring mapName = L"Map\\mapa";
	if (cmdLine[0] != 0)
	{
		mapName = std::wstring(L"Map\\") + std::wstring(cmdLine);
	}
	m_dir2D.Init(hWnd, (TCHAR*)mapName.c_str());

#if 0
	GenerateLogBitmaps(L"LogFile-4118.log", L"Bitmaps");
	//GenerateLogFile(L"LogFile-233.log", L"mat1.txt");
	PostQuitMessage(0);
	return;
#endif


#if 0
	// open GPS port
	if (!m_SerialGPS.Init(SERIAL_PORT_GPS, 9600))
	{
		MessageBox(hWnd, L"Can't open GPS Port", SERIAL_PORT_GPS, MB_ICONWARNING);
	}
	else
	{		
		m_GPS.Init();

		// send GPS init commands
		BYTE packet[5000];
		
		int toSend = m_GPS.GenerateMsgCFGPrt(packet, 115200); // set to 115200
		m_SerialGPS.Write(packet, toSend);
		
		Sleep(100); // flush?
		m_SerialGPS.Close();
		m_SerialGPS.Init(SERIAL_PORT_GPS, 115200);
		
		toSend = m_GPS.GenerateMsgCFGRate(packet, 100);
		m_SerialGPS.Write(packet, toSend);
		toSend = m_GPS.GenerateMsgCFGMsg(packet, 0x01, 0x07, 1);
		m_SerialGPS.Write(packet, toSend);
		toSend = m_GPS.GenerateMsgNAV5Msg(packet, 6, 2); // airborne <1g, 3D mode only
		//toSend = m_GPS.GenerateMsgNAV5Msg(packet, 7, 3); // airborne <2g, 2D/3D mode
		m_SerialGPS.Write(packet, toSend);

		// check response
		Sleep(100); // flush?
		int rd = m_SerialGPS.Read(packet);
		m_GPS.NewRXPacket(packet, rd);
	}
#endif

	// init filters
	m_FilterControlStationRSSI.Init(100);
	m_FilterKikiRSSI.Init(100);
	m_FilterFuelPercent.Init(100);
	m_FilterVertSpeed.Init(30);
	m_FilterSpeed.Init(30);

	// open log file
	TCHAR filename[100];
	swprintf_s(filename, 100, L"LogFile-%d.log", (int)CPerformanceTimer::GetCurrentTimestamp());
	m_LogFile.open(filename, std::ios::binary);
}

void CApplication::Shutdown()
{	
	// close log file
	m_LogFile.close();

	m_dir2D.Shutdown();
}

void CApplication::OnTimer()
{
	m_TimerCounter++;
	/*
	// send ping (range/ check)
	if ((m_TimerCounter % 10) == 0)
	{
		BYTE datatoSend[] = { 0x10 }; // PING
		BYTE packet[120]; // max size is 120!
		int toSend = m_XBee.GenerateTXPacket(datatoSend, 1, packet);
		m_Serial.Write(packet, toSend);
	}

	//  receive new data
	BYTE buffer[5000];
	int rd = m_Serial.Read(buffer);
	if( rd > 0) m_XBee.NewRXPacket(buffer, rd);
	*/
	// Receive data from network!!!
	// TODO!!!


	// process GPS
	//rd = m_SerialGPS.Read(buffer);
	//if (rd > 0) m_GPS.NewRXPacket(buffer, rd);

	// fill data and draw
	SUserData drawData;
	memset(&drawData, 0, sizeof(drawData));
	/*
	drawData.LoopCounter = m_RXData.LoopCounter;
	drawData.ActualMode = m_RXData.ActualMode;
	drawData.ModeRequest = m_ModeRequest;
	drawData.Roll = m_RXData.Roll;
	drawData.Pitch = m_RXData.Pitch;
	drawData.Yaw = m_RXData.Yaw;

	drawData.dRoll = m_RXData.dRoll;
	drawData.dPitch = m_RXData.dPitch;
	drawData.dYaw = m_RXData.dYaw;

	drawData.Pressure = m_RXData.Pressure;	
	drawData.Altitude = m_RXData.Altitude;
	//drawData.Vertspeed = m_RXData.Vertspeed;
	drawData.Vertspeed = m_FilterVertSpeed.Add(m_RXData.Vertspeed);
	//drawData.FuelLevel = m_RXData.FuelLevel;
	drawData.FuelLevel = m_FilterFuelPercent.Add(m_RXData.FuelLevel);
	memcpy(drawData.MotorThrusts, m_RXData.MotorThrusts, 4);

	// gps
	drawData.GPSTime = m_RXData.GPSTime;
	drawData.FixType = m_RXData.FixType;
	drawData.FixFlags = m_RXData.FixFlags;
	drawData.NumSV = m_RXData.NumSV;
	drawData.Longitude = m_RXData.Longitude;
	drawData.Latitude = m_RXData.Latitude;
	drawData.HeightMSL = m_RXData.HeightMSL;
	drawData.HorizontalAccuracy = m_RXData.HorizontalAccuracy;
	drawData.VerticalAccuracy = m_RXData.VerticalAccuracy;
	drawData.VelN = m_RXData.VelN;
	drawData.VelE = m_RXData.VelE;
	drawData.VelD = m_RXData.VelD;
	drawData.SpeedAcc = m_RXData.SpeedAcc;
	//drawData.Speed = (float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f; // [km/h!!!]
	drawData.Speed = m_FilterSpeed.Add((float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f); // [km/h!!!]

	drawData.RXKikiFrameCount = m_RXData.RXFrameCount;
	drawData.RXControlStationFrameCount = m_RXPacketCounter;
	drawData.RXKikiRSSI = (int)m_FilterKikiRSSI.Add(-(float)m_RXData.RSSI); // filter RSSI
	drawData.RXControlStationRSSI = (int)m_FilterControlStationRSSI.Add(-(float)m_RXRSSI); // filter RSSI

	drawData.LocalTime = CPerformanceTimer::GetCurrentTimestamp();
	*/
	m_dir2D.Draw(drawData, m_NoTelemetry);

	// Save Data To File
	m_LogFile.write((const char*)&drawData, sizeof(SUserData));
}

void CApplication::GenerateLogBitmaps(std::wstring logFilename, std::wstring destination)
{
	std::ifstream file(logFilename, std::ios::binary);
	
	SUserData data;	
	file.read((char*)&data, sizeof(SUserData)); // read first chunk
	double timestamp = data.LocalTime; // start time
	double frameTime = 1 / 30.00; // 30.00 frames per second (Mobius)
	int index = 0;

	while (!file.eof())
	{
		SUserData nextData;
		file.read((char*)&nextData, sizeof(SUserData));
		double dataTime = nextData.LocalTime;

		while (timestamp < dataTime)
		{
			if (data.LoopCounter > 70000) // Ignore begining
			{
				m_dir2D.Draw(data, false); // display! (data)
				TCHAR filename[100];
				swprintf_s(filename, 100, L"%s//image-%d.png", destination.c_str(), index++);
				m_dir2D.DrawToBitmap(data, filename);
			}
			timestamp += frameTime;
		}
		data = nextData;
	};

	file.close();
}

void CApplication::GenerateLogFile(std::wstring logFilename, std::wstring destination)
{
	std::ifstream file(logFilename, std::ios::binary);
	std::ofstream logFile(destination, std::ios::binary);

	logFile << "Loop " << "Mode " << "NumSV " << "FuelLevel " << "Roll " << "Pitch " << "Yaw " << "dRoll " << "dPitch " << "dYaw " << "T1 " << "T2 " << "T3 " << "T4 " << "Altitude " << "VertSpeed " << "Pressure " << "MSL " << "VelN " << "VelE " << "VelD " << "HorAcc " << std::endl;

	do
	{
		SUserData data;
		file.read((char*)&data, sizeof(SUserData));
		
		// dump to file
		CHAR buf[500];
		sprintf_s(buf, 500, "%d %d %d %0.2f", data.LoopCounter, data.ActualMode, data.NumSV, data.FuelLevel);
		sprintf_s(buf, 500, "%s %0.2f %0.2f %0.2f", buf, data.Roll, data.Pitch, data.Yaw);
		sprintf_s(buf, 500, "%s %0.2f %0.2f %0.2f", buf, data.dRoll, data.dPitch, data.dYaw);
		sprintf_s(buf, 500, "%s %d %d %d %d", buf, data.MotorThrusts[0], data.MotorThrusts[1], data.MotorThrusts[2], data.MotorThrusts[3]);
		sprintf_s(buf, 500, "%s %0.2f %0.2f %0.2f %0.2f", buf, data.Altitude, data.Vertspeed, data.Pressure, data.HeightMSL / 1000.0);
		sprintf_s(buf, 500, "%s %0.3f %0.3f %0.3f %0.3f", buf, data.VelN / 1000.0, data.VelE / 1000.0, data.VelD / 1000.0, data.HorizontalAccuracy / 1000.0);
		logFile << buf << std::endl;


	} while ((!file.eof()));

	file.close();
	logFile.close();
}

// RX callback (from m_XBee.NewRXPacket() )
void CApplication::NewPacketReceived(BYTE* data, int len, int rssi, int frametype)
{
	if (frametype == 0x80)
	{
		// DATA
		m_RXRSSI = rssi;
		
		if (len == sizeof(m_RXEthData))
		{
			// fill data
			memcpy(&m_RXEthData, data, len);
			m_RXPacketCounter++;
		}
		
	}
	else if (frametype == 0x88)
	{
		// AT Response (used for Channel SCAN)
		int channelNoise[12];
		std::wstring channelsTxt;
		for (int i = 0; i != 12; i++)
		{
			channelNoise[i] = -data[i + 4];
			TCHAR buf[100];
			swprintf_s(buf, 100, L"Ch 0x%02X: %d\n", i + 0x0C, channelNoise[i]);
			channelsTxt += std::wstring(buf);
		}
		MessageBox(NULL, channelsTxt.c_str(), L"Noise Levels", MB_ICONASTERISK);
	}
}

void CApplication::DownloadWaypoints(SWaypoint* wps, int cnt)
{
	STXWaypoints data;
	data.WaypointCnt = cnt;
	for (int i = 0; i != cnt; i++)
	{
		data.waypoints[i].Altitude = wps[i].Altitude;
		data.waypoints[i].Latitude = (int)(wps[i].Latitude * 1e7);
		data.waypoints[i].Longitude = (int)(wps[i].Longitude * 1e7);
	}
	int len = sizeof(STXWaypoints);
	//BYTE packet[120]; // max size is 120!
	//int toSend = m_XBee.GenerateTXPacket((BYTE*)&data, len, packet);
	//m_Serial.Write(packet, toSend);
	//m_Serial.Write(packet, toSend);
}

void CApplication::ExecuteTrajectory(float velocity)
{
	STXGotoExecute data;
	data.Command = 0x02; // execute trajectory
	data.Velocity = velocity;
	int len = sizeof(STXGotoExecute);
	//BYTE packet[120]; // max size is 120!
	//int toSend = m_XBee.GenerateTXPacket((BYTE*)&data, len, packet);
	//m_Serial.Write(packet, toSend);
	//m_Serial.Write(packet, toSend);
}

void CApplication::ExecuteTarget(SWaypoint target)
{
	STXGotoExecute data;
	data.Command = 0x01; // execute target
	data.TargetWaypoint.Altitude = target.Altitude;
	data.TargetWaypoint.Latitude = (int)(target.Latitude * 1e7);
	data.TargetWaypoint.Longitude = (int)(target.Longitude * 1e7);
	int len = sizeof(STXGotoExecute);
	//BYTE packet[120]; // max size is 120!
	//int toSend = m_XBee.GenerateTXPacket((BYTE*)&data, len, packet);
	//m_Serial.Write(packet, toSend);
	//m_Serial.Write(packet, toSend);
}

void CApplication::ExecuteOrbit(SWaypoint target, float velocity)
{
	STXGotoExecute data;
	data.Command = 0x03; // execute orbit
	data.TargetWaypoint.Altitude = target.Altitude;
	data.TargetWaypoint.Latitude = (int)(target.Latitude * 1e7);
	data.TargetWaypoint.Longitude = (int)(target.Longitude * 1e7);
	data.Velocity = velocity;
	int len = sizeof(STXGotoExecute);
	//BYTE packet[120]; // max size is 120!
	//int toSend = m_XBee.GenerateTXPacket((BYTE*)&data, len, packet);
	//m_Serial.Write(packet, toSend);
	//m_Serial.Write(packet, toSend);
}