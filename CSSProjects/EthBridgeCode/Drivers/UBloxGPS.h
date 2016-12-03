#pragma once

typedef unsigned char BYTE;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;

class UBloxGPS
{
public:
	void Init();
	void NewRXPacket(BYTE* data, int dataLen);
	int GenerateTXPacket(BYTE CLASS, BYTE ID, BYTE* data, int LENGTH, BYTE* packet);

	// message generator
	int GenerateMsgCFGPrt(BYTE* packet, int baudRate);
	int GenerateMsgCFGRate(BYTE* packet, int measRateMS);
	int GenerateMsgCFGMsg(BYTE* packet, BYTE msgClass, BYTE msgID, BYTE rate);
	int GenerateMsgNAV5Msg(BYTE* packet, BYTE dynModel, BYTE fixMode);

	// data
	int ACKCount;
	int NAKCount;
	int MsgTotal;
	int MsgParsed;

	UINT32 GPSTime; // GPS time of week [ms]
	BYTE FixType; // GNSSfix Type
	BYTE FixFlags; // FIX Flags
	BYTE NumSV; // number of active satellites
	INT32 Longitude; // 1e-7 [deg]
	INT32 Latitude; // 1e-7 [deg]
	INT32 HeightMSL; // MSL [mm]
	UINT32 HorizontalAccuracy; // [mm]
	UINT32 VerticalAccuracy; // [mm]
	INT32 VelN; // Speed North [mm/s]
	INT32 VelE; // Speed East [mm/s]
	INT32 VelD; // Speed Down [mm/s]
	UINT32 SpeedAcc; // Speec accuracy [mm/s]
	BYTE SatCNOs[32]; // 32 sattelites cno [dB]

private:
	struct CHKSUM
	{
		BYTE CK_A;
		BYTE CK_B;
	};

	CHKSUM CalculateCheckSum(BYTE* packet, int LENGTH);
	void ParseMessage();

private:
	// RX Data
	enum ERXPhase { SYNC1, SYNC2, CLASS, ID, LENLSB, LENMSB, PAYLOAD, CHK_A, CHK_B };
	ERXPhase RXPhase;

	BYTE m_CLASS;
	BYTE m_ID;
	UINT16 m_Length;
	BYTE m_Payload[1000];
	int m_PayloadIndex;
	BYTE m_ChkA;
	BYTE m_ChkB;
	
};

