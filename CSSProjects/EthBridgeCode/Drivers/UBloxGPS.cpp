#include "UBloxGPS.h"
#include <string.h>

void UBloxGPS::Init()
{
	RXPhase = UBloxGPS::SYNC1;
	ACKCount = 0;
	NAKCount = 0;
	MsgTotal = 0;
	MsgParsed = 0;
}

void UBloxGPS::NewRXPacket(BYTE* data, int dataLen)
{
	// RX Parser
	for (int i = 0; i != dataLen; i++)
	{
		BYTE b = data[i];
		switch (RXPhase)
		{
		case UBloxGPS::SYNC1:
			if (b == 0xB5)
			{
				MsgTotal++;
				RXPhase = UBloxGPS::SYNC2; // wait for start
			}
			break;

		case UBloxGPS::SYNC2:
			if (b == 0x62)
			{
				// reset chksum
				m_ChkA = 0;
				m_ChkB = 0;
				RXPhase = UBloxGPS::CLASS;
			}
			else RXPhase = UBloxGPS::SYNC1; // reset
			break;

		case UBloxGPS::CLASS:
			m_CLASS = b;
			m_ChkA = m_ChkA + b;
			m_ChkB = m_ChkB + m_ChkA;
			RXPhase = UBloxGPS::ID;
			break;

		case UBloxGPS::ID:
			m_ID = b;
			m_ChkA = m_ChkA + b;
			m_ChkB = m_ChkB + m_ChkA;
			RXPhase = UBloxGPS::LENLSB;
			break;

		case UBloxGPS::LENLSB:
			m_Length = b;
			m_ChkA = m_ChkA + b;
			m_ChkB = m_ChkB + m_ChkA;
			RXPhase = UBloxGPS::LENMSB;
			break;

		case UBloxGPS::LENMSB:
			m_Length = m_Length + (b * 256);
			m_PayloadIndex = 0;
			m_ChkA = m_ChkA + b;
			m_ChkB = m_ChkB + m_ChkA;
			if (m_Length == 0) RXPhase = UBloxGPS::CHK_A; // skip payload
			else RXPhase = UBloxGPS::PAYLOAD;
			break;

		case UBloxGPS::PAYLOAD:
			m_Payload[m_PayloadIndex] = b;
			m_PayloadIndex++;
			m_ChkA = m_ChkA + b;
			m_ChkB = m_ChkB + m_ChkA;
			if (m_PayloadIndex >= m_Length) RXPhase = UBloxGPS::CHK_A;
			break;

		case UBloxGPS::CHK_A:
			if (m_ChkA == b) RXPhase = UBloxGPS::CHK_B;
			else RXPhase = UBloxGPS::SYNC1; // checksum failed->reset
			break;

		case UBloxGPS::CHK_B:
			if (m_ChkB == b)
			{
				// OK
				ParseMessage();
				MsgParsed++;
			}
			else
			{
				// checksum error!
			}
			RXPhase = UBloxGPS::SYNC1; // reset
			break;
		}
	}
}
 
int UBloxGPS::GenerateTXPacket(BYTE CLASS, BYTE ID, BYTE* data, int LENGTH, BYTE* packet)
{
	// assemble header
	packet[0] = 0xB5; // SYNC1
	packet[1] = 0x62; // SYNC2
	packet[2] = CLASS; // CLASS
	packet[3] = ID; // ID
	packet[4] = (BYTE)(LENGTH % 256); 
	packet[5] = (BYTE)(LENGTH / 256);
	
	// copy payload
	memcpy(&packet[6], data, LENGTH);

	// add checksum
	CHKSUM chk = CalculateCheckSum(packet, LENGTH);
	packet[LENGTH + 6] = chk.CK_A;
	packet[LENGTH + 7] = chk.CK_B;

	return LENGTH + 8; // return total length (2xSYNC + CLASS + ID + 2xLEN + 2xCHKSUM + LENGTH)
}

// packet is whole packet (starts with SYNC1), chk "data" is 4 bytes longer
UBloxGPS::CHKSUM UBloxGPS::CalculateCheckSum(BYTE* packet, int LENGTH)
{
	BYTE ck_A = 0;
	BYTE ck_B = 0;

	for (int i = 2; i != LENGTH + 4 + 2; i++)
	{
		ck_A = ck_A + packet[i];
		ck_B = ck_B + ck_A;
	}

	CHKSUM chk;
	chk.CK_A = ck_A;
	chk.CK_B = ck_B;

	return chk;
}

// Custom TX Messages
int UBloxGPS::GenerateMsgCFGRate(BYTE* packet, int measRateMS)
{
	BYTE localBuffer[100];

	UINT16 measRate = measRateMS;
	UINT16 navRate = 1; // must be 1
	UINT16 timeRef = 1; // GPS Time
	memcpy(&localBuffer[0], &measRate, sizeof(UINT16));
	memcpy(&localBuffer[2], &navRate, sizeof(UINT16));
	memcpy(&localBuffer[4], &timeRef, sizeof(UINT16));

	return GenerateTXPacket(0x06, 0x08, localBuffer, 6, packet);
}

int UBloxGPS::GenerateMsgCFGPrt(BYTE* packet, int baudRate)
{
	BYTE localBuffer[100];

	BYTE portID = 1; // UART1
	localBuffer[0] = portID;
	BYTE res0 = 0;
	localBuffer[1] = res0;
	UINT16 txReady = 0;
	memcpy(&localBuffer[2], &txReady, sizeof(UINT16));
	UINT32 mode = 0x000008D0;
	memcpy(&localBuffer[4], &mode, sizeof(UINT32));
	UINT32 baudRat = baudRate;
	memcpy(&localBuffer[8], &baudRat, sizeof(UINT32));
	UINT16 inProtoMask = 0x07; // RTCM+NMEA+UBX
	memcpy(&localBuffer[12], &inProtoMask, sizeof(UINT16));
	UINT16 outProtoMask = 0x01; // UBX
	memcpy(&localBuffer[14], &outProtoMask, sizeof(UINT16));
	UINT16 flags = 0x00; // no extended timeout
	memcpy(&localBuffer[16], &flags, sizeof(UINT16));
	UINT16 res5 = 0x00; 
	memcpy(&localBuffer[18], &res5, sizeof(UINT16));

	return GenerateTXPacket(0x06, 0x00, localBuffer, 20, packet);
}

int UBloxGPS::GenerateMsgCFGMsg(BYTE* packet, BYTE msgClass, BYTE msgID, BYTE rate)
{
	BYTE localBuffer[100];

	localBuffer[0] = msgClass;
	localBuffer[1] = msgID;
	localBuffer[2] = rate;

	return GenerateTXPacket(0x06, 0x01, localBuffer, 3, packet);
}

int UBloxGPS::GenerateMsgNAV5Msg(BYTE* packet, BYTE dynModel, BYTE fixMode)
{
	BYTE localBuffer[100];

	UINT16 mask = 0x0005; // (dynamic model settings + fix mode settings)
	memcpy(&localBuffer[0], &mask, sizeof(UINT16));
	localBuffer[2] = dynModel;
	localBuffer[3] = fixMode;
	memset(&localBuffer[4], 0x00, 32); // set unused fields to zero

	return GenerateTXPacket(0x06, 0x24, localBuffer, 36, packet);
}

// RX Messages
void UBloxGPS::ParseMessage()
{
	BYTE *pyld = m_Payload;

	switch (m_CLASS)
	{

	case 0x05: // ACK
		if (m_ID == 0x01)
		{
			// ACK-ACK
			ACKCount++;
		}
		if (m_ID == 0x00)
		{
			// ACK-NAK
			NAKCount++;
		}
		break;

	case 0x01: // NAV
		if (m_ID == 0x07)
		{
			//NAV-PVT
			UINT32 iTOW; // GPS time of week [ms]
			memcpy(&iTOW, &pyld[0], sizeof(iTOW));			
			BYTE fixType; // GNSSfix Type
			fixType = pyld[20];
			BYTE flags; // FIX Flags
			flags = pyld[21];
			BYTE numSV; // number of active satellites
			numSV = pyld[23];
			INT32 longitude; // 1e-7 [deg]
			memcpy(&longitude, &pyld[24], sizeof(longitude));
			INT32 latitude; // 1e-7 [deg]
			memcpy(&latitude, &pyld[28], sizeof(latitude));
			INT32 heightMSL; // MSL [mm]
			memcpy(&heightMSL, &pyld[36], sizeof(heightMSL));
			UINT32 horizontalAccuracy; // [mm]
			memcpy(&horizontalAccuracy, &pyld[40], sizeof(horizontalAccuracy));
			UINT32 verticalAccuracy; // [mm]
			memcpy(&verticalAccuracy, &pyld[44], sizeof(verticalAccuracy));
			INT32 velN; // Speed North [mm/s]
			memcpy(&velN, &pyld[48], sizeof(velN));
			INT32 velE; // Speed East [mm/s]
			memcpy(&velE, &pyld[52], sizeof(velE));
			INT32 velD; // Speed Down [mm/s]
			memcpy(&velD, &pyld[56], sizeof(velD));
			UINT32 sAcc; // Speec accuracy [mm/s]
			memcpy(&sAcc, &pyld[68], sizeof(sAcc));

			GPSTime = iTOW;
			FixType = fixType;
			FixFlags = flags;
			NumSV = numSV;
			Longitude = longitude;
			Latitude = latitude;
			HeightMSL = heightMSL;
			HorizontalAccuracy = horizontalAccuracy;
			VerticalAccuracy = verticalAccuracy;
			VelN = velN;
			VelE = velE;
			VelD = velD;
			SpeedAcc = sAcc;
		}

		if (m_ID == 0x35)
		{
			// NAV-SAT
			BYTE numSvs;
			numSvs = pyld[5];

			// clear array
			memset(SatCNOs, 0, sizeof(SatCNOs));

			for(int i=0; i!=numSvs; i++)
			{
				//BYTE gnssId, svId;
				BYTE cno;
				//gnssId =  pyld[12*i + 8];
				//svId =  pyld[12*i + 9];
				cno =  pyld[12*i + 10];

				if( i < 32)
				{
					SatCNOs[i] = cno;
				}
			}
		}
		break;
	}
}
