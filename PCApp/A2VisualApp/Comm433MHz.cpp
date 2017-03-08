#include "stdafx.h"
#include "Comm433MHz.h"
#include <string.h>
#include "CRC32.h"

void CComm433MHz::Init()
{
	RXPhase = CComm433MHz::HDR_FF;
	MsgReceivedOK = 0;
	CrcErrors = 0;
	HeaderFails = 0;
}

void CComm433MHz::NewRXPacket(BYTE* data, int dataLen, ReceivedMessageCallbackType ProcessMessage)
{
	// RX Parser
	for (int i = 0; i != dataLen; i++)
	{
		BYTE b = data[i];
		switch (RXPhase)
		{
			// HEADER
		case CComm433MHz::HDR_FF:
			if (b == 0xFF)
			{
				RXPhase = CComm433MHz::HDR_FE; // wait for start
			}
			break;

		case CComm433MHz::HDR_FE:
			if (b == 0xFE)
			{
				RXPhase = CComm433MHz::HDR_A5; // wait for start
			}
			else
			{
				RXPhase = CComm433MHz::HDR_FF; // reset
				HeaderFails++;
			}
			break;

		case CComm433MHz::HDR_A5:
			if (b == 0xA5)
			{
				RXPhase = CComm433MHz::HDR_5A; // wait for start
			}
			else
			{
				RXPhase = CComm433MHz::HDR_FF; // reset
				HeaderFails++;
			}
			break;

		case CComm433MHz::HDR_5A:
			if (b == 0x5A)
			{
				RXPhase = CComm433MHz::TYPE; // wait for start
			}
			else
			{
				RXPhase = CComm433MHz::HDR_FF; // reset
				HeaderFails++;
			}
			break;

		case CComm433MHz::TYPE:
			Type = b;
			RXPhase = CComm433MHz::LEN;
			break;

		case CComm433MHz::LEN:
			Len = b;
			m_DataIndex = 0; // reset data counter
			RXPhase = CComm433MHz::DATA;
			break;


		case CComm433MHz::DATA:
			m_Data[m_DataIndex] = b;
			m_DataIndex++;
			if (m_DataIndex >= Len) RXPhase = CComm433MHz::CRC_A;
			break;

		case CComm433MHz::CRC_A:
			CRC = (UINT)(b << 24);
			RXPhase = CComm433MHz::CRC_B;
			break;

		case CComm433MHz::CRC_B:
			CRC += (UINT)(b << 16);
			RXPhase = CComm433MHz::CRC_C;
			break;

		case CComm433MHz::CRC_C:
			CRC += (UINT)(b << 8);
			RXPhase = CComm433MHz::CRC_D;
			break;

		case CComm433MHz::CRC_D:
			CRC += b;
			// check CRC, process data!!
			UINT calculatedCRC = CRC32::CalculateCrc32(m_Data, Len);
			if (calculatedCRC == CRC)
			{
				// message OK, process!
				MsgReceivedOK++;

				ProcessMessage(Type, m_Data, Len);
			}
			else
			{
				// CRC Failed
				CrcErrors++;
			}

			RXPhase = CComm433MHz::HDR_FF;
			break;
		}
	}
}
 
int CComm433MHz::GenerateTXPacket(BYTE Type, BYTE* Data, BYTE Len, BYTE* OutputPacket)
{
	// assemble header
	OutputPacket[0] = 0xFF; // Header
	OutputPacket[1] = 0xFE; // Header
	OutputPacket[2] = 0xA5; // Header
	OutputPacket[3] = 0x5A; // Header

	OutputPacket[4] = Type; // Type
	OutputPacket[5] = Len; // Data Length
	for (int i = 0; i != Len; i++)
	{
		OutputPacket[6 + i] = Data[i];
	}

	// CRC
	UINT calculatedCRC = CRC32::CalculateCrc32(Data, Len);
	OutputPacket[6 + Len] = (BYTE)(calculatedCRC >> 24);
	OutputPacket[6 + Len + 1] = (BYTE)(calculatedCRC >> 16);
	OutputPacket[6 + Len + 2] = (BYTE)(calculatedCRC >> 8);
	OutputPacket[6 + Len + 3] = (BYTE)(calculatedCRC);

	return 6 + Len + 4; // return total length (4xHEDER + TYPE + LEN + 4xCHKSUM)
}
