#include "SBUSComm.h"
#include <string.h>

void SBUSComm::Init()
{
    RXPhase = SBUSComm::START;
    NumberOfRecvPackets = 0;
}

void SBUSComm::NewRXPacket(BYTE* data, int dataLen)
{
	// RX Parser
    for(int i=0; i!= dataLen; i++)
    {
        BYTE b = data[i];
        switch (RXPhase)
        {
			case SBUSComm::START:
				if (b == 0x0F) // check start byte
				{
					RXCounter = 0;
					RXPhase = SBUSComm::DATA; // wait for start
				}
				break;

			case SBUSComm::DATA:
				RXData[RXCounter] = b;
				RXCounter++;
				if (RXCounter == 23) RXPhase = SBUSComm::END;
				break;

			case SBUSComm::END:
				if (b == 0x00)
				{
					// OK
					ParseMessage();
				}
				else
				{
					// Framing Error!
				}
				RXPhase = SBUSComm::START; // wait for new
				break;
		}
	}
}

// RX Messages
void SBUSComm::ParseMessage()
{
	NumberOfRecvPackets++;

	// parse data
	Channels[0] = RXData[0] + ((RXData[1] & 0x07) << 8);
	Channels[1] = (RXData[1]>>3) + ((RXData[2]&0x3F) << 5);
	Channels[2] = (RXData[2]>>6) + ((RXData[3]&0xFF) << 2) + ((RXData[4]&0x01) << 10);
	Channels[3] = (RXData[4]>>1) + ((RXData[5]&0x0F) << 7);
	Channels[4] = (RXData[5]>>4) + ((RXData[6]&0x7F) << 4);
	Channels[5] = (RXData[6]>>7) + ((RXData[7]&0xFF) << 1) + ((RXData[8]&0x03) << 9);
	Channels[6] = (RXData[8]>>2) + ((RXData[9]&0x1F) << 6); // CHECK MORE...
	Channels[7] = (RXData[9]>>5) + ((RXData[10]&0xFF) << 3);

	Channels[8] = RXData[11] + ((RXData[12] & 0x07) << 8);
	Channels[9] = (RXData[12]>>3) + ((RXData[13]&0x3F) << 5);
	Channels[10] = (RXData[13]>>6) + ((RXData[14]&0xFF) << 2) + ((RXData[15]&0x01) << 10);
	Channels[11] = (RXData[15]>>1) + ((RXData[16]&0x0F) << 7);
	Channels[12] = (RXData[16]>>4) + ((RXData[17]&0x7F) << 4); // CHECK THIS CHANNEL!!!
	Channels[13] = (RXData[17]>>7) + ((RXData[18]&0xFF) << 1) + ((RXData[19]&0x03) << 9);
	Channels[14] = (RXData[19]>>2) + ((RXData[20]&0x1F) << 6);
	Channels[15] = (RXData[20]>>5) + ((RXData[21]&0xFF) << 3);

	// bit7 = ch17 = digital channel (0x80)
	// bit6 = ch18 = digital channel (0x40)
	// bit5 = Frame lost, equivalent red LED on receiver (0x20)
	// bit4 = failsafe activated (0x10)
	SystemByte = RXData[22];
}
