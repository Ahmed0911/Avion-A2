
/*
 * CommData.h
 *
 *  Created on: Oct 12, 2014
 *      Author: Sara
 */

#ifndef COMMDATA_H_
#define COMMDATA_H_

struct SCommData
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

	// engines
	unsigned char MotorThrusts[4]; // [0...100%]

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

	// Comm
	unsigned int RXFrameCount;
	unsigned int RSSI;
};

struct SCommDataSensorDiag
{
	unsigned int LoopCounter;

	// IMU
	float AccelX;	// [m/s^2]
	float AccelY;	// [m/s^2]
	float AccelZ;	// [m/s^2]
	float GyroX;	// [deg/s]
	float GyroY;	// [deg/s]
	float GyroZ;	// [deg/s]
	float MagX;		// ???
	float MagY;		// ???
	float MagZ;		// ???
	float Roll;		// [deg]
	float Pitch;	// [deg]
	float Yaw;		// [deg]
};

struct STXCommModeSet
{
	unsigned char Mode;
	unsigned char dummy1;
	unsigned char dummy2;
	unsigned char dummy3;
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
