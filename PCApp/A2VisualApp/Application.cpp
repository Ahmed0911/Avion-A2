#include "stdafx.h"
#include "Application.h"

#include "CommData.h"
#include "Timer.h"
#include "CRC32.h"
#include <fstream>

#define ETHERNET_PORT_GATEWAY "10.0.1.101"
#define ETHERNET_PORT_A2 "10.0.1.121"

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

	// Open ethernet port
	m_ethernetCommGW.Init(8000);
	m_ethernetCommA2.Init(8100);
	m_ethernetCommGW.ConnectTo(ETHERNET_PORT_GATEWAY, NewPacketRxWrapper);
	m_ethernetCommA2.ConnectTo(ETHERNET_PORT_A2, NewPacketRxWrapper);

	// init filters
	m_FilterControlStationRSSI.Init(100);
	m_FilterA2RSSI.Init(100);
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
	
	// Receive data from network!!!
	m_ethernetCommGW.Update();
	//m_ethernetCommA2.Update();


	// fill data and draw
	SUserData drawData;
	memset(&drawData, 0, sizeof(drawData));
	//drawData.LoopCounter = m_RXHopeRFData.LoopCounter;
	drawData.LoopCounter = m_RXGatewayData.LoopCounter;
	drawData.ActualMode = m_RXHopeRFData.ActualMode;
	drawData.Roll = m_RXHopeRFData.Roll;
	drawData.Pitch = m_RXHopeRFData.Pitch;
	drawData.Yaw = m_RXHopeRFData.Yaw;

	drawData.dRoll = m_RXHopeRFData.dRoll;
	drawData.dPitch = m_RXHopeRFData.dPitch;
	drawData.dYaw = m_RXHopeRFData.dYaw;

	//drawData.Pressure = m_RXHopeRFData.Pressure;
	drawData.Altitude = m_RXHopeRFData.Altitude;
	//drawData.Vertspeed = m_RXData.Vertspeed;
	drawData.Vertspeed = m_FilterVertSpeed.Add(m_RXHopeRFData.Vertspeed);
	//drawData.FuelLevel = m_RXData.FuelLevel;
	drawData.FuelLevel = m_FilterFuelPercent.Add(m_RXHopeRFData.BatteryVoltage);
	//memcpy(drawData.MotorThrusts, m_RXData.MotorThrusts, 4);

	// gps
	//drawData.GPSTime = m_RXHopeRFData.GPSTime;
	drawData.FixType = m_RXHopeRFData.FixType;
	//drawData.FixFlags = m_RXHopeRFData.FixFlags;
	drawData.NumSV = m_RXHopeRFData.NumSV;
	drawData.Longitude = m_RXHopeRFData.Longitude;
	drawData.Latitude = m_RXHopeRFData.Latitude;
	//drawData.HeightMSL = m_RXHopeRFData.HeightMSL;
	//drawData.HorizontalAccuracy = m_RXHopeRFData.HorizontalAccuracy;
	//drawData.VerticalAccuracy = m_RXHopeRFData.VerticalAccuracy;
	drawData.VelN = m_RXHopeRFData.VelN;
	drawData.VelE = m_RXHopeRFData.VelE;
	//drawData.VelD = m_RXHopeRFData.VelD;
	//drawData.SpeedAcc = m_RXHopeRFData.SpeedAcc;
	//drawData.Speed = (float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f; // [km/h!!!]
	drawData.Speed = m_FilterSpeed.Add((float)sqrt(drawData.VelN*drawData.VelN / 1000000.0 + drawData.VelE*drawData.VelE / 1000000.0) * 3.6f); // [km/h!!!]

	//drawData.RXKikiFrameCount = m_RXHopeRFData.RXFrameCount;
	drawData.RXControlStationFrameCount = m_RXPacketCounter;
	drawData.RXA2RSSI = (int)m_FilterA2RSSI.Add(-(float)m_RXHopeRFData.HopeRXRSSI); // filter RSSI
	drawData.RXControlStationRSSI = (int)m_FilterControlStationRSSI.Add(-(float)m_RXHopeRFData.HopeTXRSSI); // filter RSSI

	drawData.LocalTime = CPerformanceTimer::GetCurrentTimestamp();

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
			break;
		}

		case 0x41:
		{
			// relayed data (HopeRF from A2)
			if (len == sizeof(m_RXHopeRFData))
			{
				SCommHopeRFDataA2Avion commDataHopeRF;
				memcpy(&commDataHopeRF, data, sizeof(commDataHopeRF));
				//ReceivedHopeRFCounter++;
				

				// check checksum
				unsigned int crcSum = CRC32::CalculateCrc32((BYTE*)&commDataHopeRF, sizeof(commDataHopeRF)-sizeof(commDataHopeRF.CRC32));
				if (crcSum == commDataHopeRF.CRC32) // CRC OK?
				{
					memcpy(&m_RXHopeRFData, &commDataHopeRF, sizeof(commDataHopeRF)); // data valid, copy to internal structure
					m_RXHopeRFData.HopeTXRSSI = m_RXGatewayData.HopeRXRSSI; // fill with received/gatewayed Hope RSSI 

					// save to file
					//byte[] arrayToWrite = Comm.GetBytes(commDataHopeRF);
					//logStream.Write(arrayToWrite, 0, arrayToWrite.Length);
				}
				//else ReceivedHopeRFCounterCrcErrors++;
			}
					 
			break;
		}
			/*			
			else if (withoutHeader.Length == Marshal.SizeOf(new SParameters())) // check size
			{
				// Parameters structure
				SParameters parametersDataHopeRF = (SParameters)Comm.FromBytes(withoutHeader, new SParameters());

				// check checksum
				uint crcSum = Crc32.CalculateCrc32(withoutHeader, withoutHeader.Length - sizeof(uint));
				if (crcSum == parametersDataHopeRF.CRC32) // CRC OK?
				{
					// Display
					formMain.DisplayParams(parametersDataHopeRF);
				}
			}*/
	}

	/*if (frametype == 0x80)
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
	}*/
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