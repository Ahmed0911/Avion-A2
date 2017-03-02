#include "stdafx.h"
#include "Application.h"

#include "CommData.h"
#include "Timer.h"
#include "CRC32.h"
#include <fstream>

#define ETHERNET_PORT_A2 L"10.0.1.121"
#define SERIAL_PORT_A2 L"\\\\.\\COM7"

// Callback wrapper
void NewPacketRxWrapper(char type, BYTE* data, int len)
{
	CApplication::getInstance()->NewPacketReceived(type, data, len);
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
	m_NoTelemetry = false;
	m_lastTelemetryReceivedTimestamp = CPerformanceTimer::GetCurrentTimestamp();
	m_RXHopeRFPacketCounter = 0;
	m_RXHopeRFCRCErrorCounter = 0;

	// create D2D
	std::wstring mapName = L"Map\\map-45.7996-15.84655-16-X.jpg";
	if (cmdLine[0] != 0)
	{
		mapName = std::wstring(L"Map\\") + std::wstring(cmdLine);
	}
	m_dir2D.Init(hWnd, (TCHAR*)mapName.c_str());

	// init filters
	m_FilterControlStationRSSI.Init(100);
	m_FilterA2RSSI.Init(100);
	m_FilterFuelPercent.Init(100);
	m_FilterVertSpeed.Init(30);
	m_FilterSpeed.Init(30);

#if 0
	GenerateLogBitmapsHopeRF(L"Log\\LogHopeRF-15-15-13.bin", L"Bitmaps");
	//GenerateLogBitmaps(L"Log\\LogFileDraw-4118.log", L"Bitmaps");
	//GenerateLogFile(L"Log\\LogFileDraw-233.log", L"mat1.txt");
	PostQuitMessage(0);
	return;
#endif

	// Open ethernet port
	m_ethernetCommA2.Init(8100);
	m_ethernetCommA2.ConnectTo(ETHERNET_PORT_A2, NewPacketRxWrapper);

	m_serialCommA2.Init();
	m_serialCommA2.ConnectTo(SERIAL_PORT_A2, NewPacketRxWrapper);

	// open log file
	TCHAR filename[100];
	swprintf_s(filename, 100, L"Log\\LogFileDraw-%d.log", (int)CPerformanceTimer::GetCurrentTimestamp());
	m_LogFileDraw.open(filename, std::ios::binary);
	swprintf_s(filename, 100, L"Log\\LogHopeRF-%d.log", (int)CPerformanceTimer::GetCurrentTimestamp());
	m_LogFileHopeRF.open(filename, std::ios::binary);
}

void CApplication::Shutdown()
{	
	// close log file
	m_LogFileHopeRF.close();
	m_LogFileDraw.close();

	m_dir2D.Shutdown();
}

void CApplication::OnTimer()
{
	m_TimerCounter++;
	
	// Receive data from network!!!
	m_serialCommA2.Update();
	//m_ethernetCommA2.Update();

	// check telemetry timestamps, mark m_NoTelemetry
	double currentTimestamp = CPerformanceTimer::GetCurrentTimestamp();
	if ((currentTimestamp - m_lastTelemetryReceivedTimestamp) < 2.0) m_NoTelemetry = false;
	//else m_NoTelemetry = true;

	// fill data and draw
	SUserData drawData;
	memset(&drawData, 0, sizeof(drawData));	
	FillHopeRFData(drawData);
	//FillEthernetData(drawData);
	m_dir2D.Draw(drawData, m_NoTelemetry);

	// Save Data To File
	m_LogFileDraw.write((const char*)&drawData, sizeof(SUserData));
}



// RX callback (from Ethernet.NewRXPacket() )
void CApplication::NewPacketReceived(char type, BYTE* data, int len)
{
	switch (type)
	{
		case 0x20:
		{
			// GW Data(from gateway)  
			if (len == sizeof(m_RXGatewayData))
			{
				memcpy(&m_RXGatewayData, data, sizeof(m_RXGatewayData));
			}
			else if (len == sizeof(m_RXHopeRFData))
			{
				// data from Lora Modem (serial)
				memcpy(&m_RXHopeRFData, data, sizeof(m_RXHopeRFData));
				m_RXHopeRFPacketCounter++;

				// data received, send ping
				//SendPing();

				// reset timestamp
				m_lastTelemetryReceivedTimestamp = CPerformanceTimer::GetCurrentTimestamp();

				// Save Data To File (HopeRF Log file, compatible with AvionA2 App)
				m_LogFileHopeRF.write((const char*)&m_RXHopeRFData, sizeof(SCommHopeRFDataA2Avion));
			}
			break;
		}

		case 0x62:
		{
			// Parameters structure
			//SParameters parametersDataHopeRF = (SParameters)Comm.FromBytes(data, new SParameters());

			// Display
			//formMain.DisplayParams(parametersDataHopeRF);
			break;
		}
	}
}

void CApplication::SendPing()
{
	BYTE toSend[] = { 1, 2, 3, 4 }; // dummy
	m_serialCommA2.SendData(0x10, toSend, 4);
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


void CApplication::FillHopeRFData(SUserData& drawData)
{
	drawData.LoopCounter = m_RXHopeRFData.LoopCounter;
	
	// IMU
	drawData.Roll = m_RXHopeRFData.Roll;
	drawData.Pitch = m_RXHopeRFData.Pitch;
	drawData.Yaw = m_RXHopeRFData.Yaw;
	drawData.dRoll = m_RXHopeRFData.dRoll;
	drawData.dPitch = m_RXHopeRFData.dPitch;
	drawData.dYaw = m_RXHopeRFData.dYaw;


	// alt/speed
	drawData.Altitude = m_RXHopeRFData.Altitude;
	//drawData.Vertspeed = m_RXHopeRFData.Vertspeed; // unfiltered
	drawData.Vertspeed = m_FilterVertSpeed.Add(m_RXHopeRFData.Vertspeed); // filtered

	// engines
	memcpy(drawData.MotorThrusts, m_RXHopeRFData.MotorThrusts, 4);

	// Voltage/Current
	drawData.BatteryVoltage = m_RXHopeRFData.BatteryVoltage;
	drawData.BatteryCurrentA = m_RXHopeRFData.BatteryCurrentA;
	drawData.BatteryTotalCharge_mAh = m_RXHopeRFData.BatteryTotalCharge_mAh;
	drawData.FuelLevel = (2000 - m_RXHopeRFData.BatteryTotalCharge_mAh)/2000*100;
	
	// GPS
	drawData.NumSV = m_RXHopeRFData.NumSV;
	drawData.FixType = m_RXHopeRFData.FixType;
	drawData.ActualMode = m_RXHopeRFData.ActualMode;
	drawData.Longitude = m_RXHopeRFData.Longitude;
	drawData.Latitude = m_RXHopeRFData.Latitude;
	drawData.VelN = m_RXHopeRFData.VelN;
	drawData.VelE = m_RXHopeRFData.VelE;
	//drawData.Speed = (float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f; // [km/h!!!]
	drawData.Speed = m_FilterSpeed.Add((float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f); // [km/h!!!]

	// RX/TX. HopeRF Data
	drawData.RXA2RSSIFrameCount = m_RXHopeRFData.HopeRXFrameCount;
	drawData.RXControlStationFrameCount = m_RXHopeRFPacketCounter;
	drawData.RXControlStationFrameErrorCount = m_RXHopeRFCRCErrorCounter;
	drawData.RXA2RSSI = (int)m_FilterA2RSSI.Add((float)m_RXHopeRFData.HopeRXRSSI); // filter RSSI
	drawData.RXControlStationRSSI = (int)m_FilterControlStationRSSI.Add((float)m_RXHopeRFData.HopeTXRSSI); // filter RSSI

	drawData.LocalTime = CPerformanceTimer::GetCurrentTimestamp();
}

// TODO!!!
void CApplication::FillEthernetData(SUserData& drawData)
{
	drawData.LoopCounter = m_RXEthData.LoopCounter;
	drawData.ActualMode = m_RXEthData.ActualMode;
	drawData.Roll = m_RXEthData.Roll;
	drawData.Pitch = m_RXEthData.Pitch;
	drawData.Yaw = m_RXEthData.Yaw;

	drawData.dRoll = m_RXEthData.dRoll;
	drawData.dPitch = m_RXEthData.dPitch;
	drawData.dYaw = m_RXEthData.dYaw;

	drawData.Pressure = m_RXEthData.Pressure;
	drawData.Altitude = m_RXEthData.Altitude;
	drawData.Vertspeed = m_RXEthData.Vertspeed;
	drawData.Vertspeed = m_FilterVertSpeed.Add(m_RXEthData.Vertspeed);
	drawData.FuelLevel = m_RXEthData.FuelLevel;
	drawData.FuelLevel = m_FilterFuelPercent.Add(m_RXEthData.BatteryVoltage);
	memcpy(drawData.MotorThrusts, m_RXEthData.MotorThrusts, 4);

	// gps
	drawData.GPSTime = m_RXEthData.GPSTime;
	drawData.FixType = m_RXEthData.FixType;
	drawData.FixFlags = m_RXEthData.FixFlags;
	drawData.NumSV = m_RXEthData.NumSV;
	drawData.Longitude = m_RXEthData.Longitude;
	drawData.Latitude = m_RXEthData.Latitude;
	drawData.HeightMSL = m_RXEthData.HeightMSL;
	drawData.HorizontalAccuracy = m_RXEthData.HorizontalAccuracy;
	drawData.VerticalAccuracy = m_RXEthData.VerticalAccuracy;
	drawData.VelN = m_RXEthData.VelN;
	drawData.VelE = m_RXEthData.VelE;
	drawData.VelD = m_RXEthData.VelD;
	drawData.SpeedAcc = m_RXEthData.SpeedAcc;
	drawData.Speed = (float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f; // [km/h!!!]
	drawData.Speed = m_FilterSpeed.Add((float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f); // [km/h!!!]

	drawData.RXA2RSSIFrameCount = m_RXEthData.HopeRXFrameCount;
	drawData.RXControlStationFrameCount = m_RXHopeRFPacketCounter;
	drawData.RXA2RSSI = (int)m_FilterA2RSSI.Add((float)m_RXEthData.HopeRXRSSI); // filter RSSI
	drawData.RXControlStationRSSI = (int)m_FilterControlStationRSSI.Add((float)m_RXEthData.HopeRXRSSI); // filter RSSI

	drawData.LocalTime = CPerformanceTimer::GetCurrentTimestamp();
}

////////////////////////////////////////////
// Log Files
// Draw/HuD log file (LogFileDraw-%d.log)
////////////////////////////////////////////
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
				m_dir2D.DrawToBitmap(data, filename, false);
			}
			timestamp += frameTime;
		}
		data = nextData;
	};

	file.close();
}

// Special HopeRF log generator (used for A2Avion app log data)
void CApplication::GenerateLogBitmapsHopeRF(std::wstring logFilename, std::wstring destination)
{
	std::ifstream file(logFilename, std::ios::binary);

	file.read((char*)&m_RXHopeRFData, sizeof(SCommHopeRFDataA2Avion)); // read first chunk
	SUserData data;
	memset(&data, 0, sizeof(SUserData));
	FillHopeRFData(data); // convert

	double timestamp = data.LoopCounter*0.0025; // start time
	double frameTime = 1 / 30.00; // 30.00 frames per second (Mobius)
	int index = 0;

	while (!file.eof())
	{
		file.read((char*)&m_RXHopeRFData, sizeof(SCommHopeRFDataA2Avion)); // read first chunk
		SUserData nextData;
		memset(&nextData, 0, sizeof(SUserData));
		FillHopeRFData(nextData); // convert
		double dataTime = nextData.LoopCounter*0.0025;

		while (timestamp < dataTime)
		{
			if (data.LoopCounter > 110000) // Ignore begining
			{
				double delta = dataTime - timestamp; // check this for no data display
				bool noData = false;
				if (delta > 1) noData = true; // NODATA

				m_dir2D.Draw(data, noData); // display! (data)
				TCHAR filename[100];
				swprintf_s(filename, 100, L"%s//image-%d.png", destination.c_str(), index++);
				m_dir2D.DrawToBitmap(data, filename, noData);
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