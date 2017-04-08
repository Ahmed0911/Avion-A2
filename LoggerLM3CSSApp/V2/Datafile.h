

#ifndef DATAFILE_H_
#define DATAFILE_H_

typedef double float64;
typedef unsigned char	BYTE;

struct SDataFile
{
	// MPU-6000
	float MPUGyroX; // [rad/s]
	float MPUGyroY;// [rad/s]
	float MPUGyroZ;  // [rad/s]
	float MPUAccX; // [m/s^2]
	float MPUAccY; // [m/s^2]
	float MPUAccZ; // [m/s^2]
	float MPUTemperature; // [°C]

	unsigned int Ticks;
	unsigned int ReceivedPackets;
	unsigned int SentPackets;

	// CAN
	unsigned int TXOverrun;
	unsigned int TXSent;

	// SDCard
	bool SDCardActive;
	unsigned int SDCardBytesWritten;
	unsigned int SDCardFails;
	unsigned int FailedQueues;
	unsigned int QueueSize;

	float64 MissionTime;
	float LoopTimeMS;
	float LoopTimeMSMAX;

	float SendPacketTimeMS;
};

struct SPingLoggerData
{
	unsigned short DestinationPort;
	unsigned short SDCardActive;
	unsigned int SDCardBytesWritten;
	unsigned int SDCardFails;
	unsigned int FailedQueues;
};

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

#endif /* DATAFILE_H_ */
