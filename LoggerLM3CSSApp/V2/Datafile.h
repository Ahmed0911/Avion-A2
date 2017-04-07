

#ifndef DATAFILE_H_
#define DATAFILE_H_

typedef double float64;

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


	float64 MissionTime;
	float LoopTimeMS;
	float LoopTimeMSMAX;

	float SendPacketTimeMS;
};

#endif /* DATAFILE_H_ */
