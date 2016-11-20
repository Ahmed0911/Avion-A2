#pragma once

typedef void (*NewPacketCallbackType)(BYTE*, int, int, int);  


class CXBeeComm
{
public:
	void Init(BYTE* destAddr, NewPacketCallbackType callback);
	void NewRXPacket(BYTE* data, int dataLen);
	int GenerateTXPacket(BYTE* data, int dataLen, BYTE* dataOut);
	int GenerateATPacket(BYTE* command, BYTE param, BYTE* dataOut);
	BYTE CalculateCheckSum(BYTE* packet, int dataLen);

private:

	BYTE DestinationAddr[8]; // 64 bit address
    NewPacketCallbackType NewPacketCallback; // callback

    // RX Data
	enum ERXPhase { START, LENMSB, LENLSB, FRAMETYPE, SOURCEADDR, RSSI, OPTIONS, DATA, CHECKSUM };
    ERXPhase RXPhase;
    int RXLength;
    int RXCounter;
    BYTE RXFrameType;
    BYTE RXSourceAddr[8];
    BYTE RXRSSI;
    BYTE RXOptions;
    BYTE RXChkSum; 
    BYTE RXData[100];
};

