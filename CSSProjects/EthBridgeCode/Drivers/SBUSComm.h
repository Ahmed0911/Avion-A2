#pragma once

typedef unsigned char BYTE;

class SBUSComm
{
public:
	void Init();
	void NewRXPacket(BYTE* data, int dataLen);

public:
	int NumberOfRecvPackets;

	unsigned int Channels[16];
	BYTE SystemByte;

private:
	void ParseMessage();

    // RX Data
	enum ERXPhase { START, DATA, END };
    ERXPhase RXPhase;
    int RXCounter;
    BYTE RXData[100];
};

