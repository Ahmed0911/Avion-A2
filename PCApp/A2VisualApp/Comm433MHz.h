#pragma once

typedef void(*ReceivedMessageCallbackType)(BYTE, BYTE*, BYTE);

class CComm433MHz
{
public:
	void Init();
	void NewRXPacket(BYTE* data, int dataLen, ReceivedMessageCallbackType callback);
	int GenerateTXPacket(BYTE Type, BYTE* Data, BYTE Len, BYTE* OutputPacket);

	// data
	int MsgReceivedOK;
	int CrcErrors;
	int HeaderFails;

private:
	// RX Data
	enum ERXPhase { HDR_FF, HDR_FE, HDR_A5, HDR_5A, TYPE, LEN, DATA, CRC_A, CRC_B, CRC_C, CRC_D };
	ERXPhase RXPhase;

	BYTE Type;
	BYTE Len;
	BYTE m_Data[200];
	int m_DataIndex;
	UINT CRC;

};

