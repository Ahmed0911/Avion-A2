
/*
 * CommData.h
 *
 *  Created on: Oct 12, 2014
 *      Author: Sara
 */

#ifndef COMMDATA_H_
#define COMMDATA_H_

struct SCommEthData
{
	unsigned int LoopCounter;

	// IMU
	float Roll;		// [deg]
	float Pitch;	// [deg]
	float Yaw;		// [deg]
	float dRoll;	// [deg/s]
	float dPitch;	// [deg/s]
	float dYaw;		// [deg/s]

	float AccX; // [m/s^2]
	float AccY; // [m/s^2]
	float AccZ; // [m/s^2]
	float MagX; // [uT]
	float MagY; // [uT]
	float MagZ; // [uT]

	// alt/speed
	float Pressure; // [Pa]
	float Temperature;// [°C]
	float Altitude; // [m]
	float Vertspeed;// [m/s]

	// engines
	unsigned char MotorThrusts[4]; // [0...100%]

	float FuelLevel; // [0...100%]
	float BatteryVoltage; // [V]
	float BatteryCurrentA; // [A]
	float BatteryTotalCharge_mAh; // [mAh]

	// GPS
	unsigned int GPSTime;
	unsigned char NumSV;
	unsigned char FixType;
	unsigned char FixFlags; // FIX Flags
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

	// Comm Eth
	unsigned int EthSentCount;
	unsigned int EthReceivedCount;
	// Comm HopeRF
	unsigned int HopeRXFrameCount;
	int HopeRXRSSI;
	int HopeRSSI;

	// Perf Stuff
	float PerfLoopTimeMS;
	float PerfCpuTimeMS;
	float PerfCpuTimeMSMAX;

	// AssistNow Data
	int AssistNextChunkToSend;

	// Tuning Data
	float TuningData[10];

	// SAT cnos
	BYTE SatCNOs[32]; // 32 sattelites cno [dB]

	// Waypoints
	int WaypointCnt;
	int WaypointDownloadCounter;

	int HomeLongitude; // 1e-7 [deg]
	int HomeLatitude; // 1e-7 [deg]

	// Launch
	unsigned short LaunchStatus1;
	unsigned short LaunchStatus2;
};

struct SCommHopeRFDataA2Avion
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
	float Altitude; // [m]
	float Vertspeed;// [m/s]

	// engines
	unsigned char MotorThrusts[4]; // [0...100%]

	// Fuel
	float BatteryVoltage; // [V]
	float BatteryCurrentA; // [A]
	float BatteryTotalCharge_mAh; // [mAh]

	// GPS
	unsigned char NumSV;
	unsigned char FixType;
	unsigned char ParamsCommandAckCnt; // Sync for params R/W
	unsigned char ActualMode; // Non GPS, but used for 32bit alignment
	int Longitude; // 1e-7 [deg]
	int Latitude; // 1e-7 [deg]
	int VelN; // Speed North [mm/s]
	int VelE; // Speed East [mm/s]

	// Comm HopeRF
	unsigned int HopeRXFrameCount;
	int HopeRXRSSI; // Received RSSI at A2 Plane (on received command)
	int HopeTXRSSI; // dummy, filled in GCS App

	// CRC
	unsigned int CRC32;
};

struct SWpt
{
	float Altitude;
	int Latitude; // 1e-7 [deg]
	int Longitude; // 1e-7 [deg]
};

struct STXWaypoints
{
	unsigned int WaypointCnt;
	SWpt waypoints[MAXWAYPOINTS];
};

struct STXGotoExecute
{
	unsigned char Command;  // 1 - Execute Target, 2 - Start Waypoints, 3 - Orbit Command
	SWpt TargetWaypoint; // target (execute target=1), orbitcenter (orbit=3)
	float Velocity; // Velocity fot waypoints and orbit
};

struct STXCommParams
{
	float Params[10];
};

#endif /* COMMDATA_H_ */
