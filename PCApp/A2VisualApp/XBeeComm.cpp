#include "stdafx.h"
#include "XBeeComm.h"
#include <string.h>

void CXBeeComm::Init(BYTE* destAddr, NewPacketCallbackType callback)
{
	memcpy(DestinationAddr, destAddr, 8);
    NewPacketCallback = callback;
    RXPhase = CXBeeComm::START;
}

void CXBeeComm::NewRXPacket(BYTE* data, int dataLen)
{
	// RX Parser
    for(int i=0; i!= dataLen; i++)
    {
        BYTE b = data[i];
        switch (RXPhase)
        {
			case CXBeeComm::START:
					if (b == 0x7E)
					{
						RXChkSum = 0;
						RXPhase = CXBeeComm::LENMSB; // wait for start
					}                        
					break;

			case CXBeeComm::LENMSB:
				RXLength = b * 256; // MSB
				RXPhase = CXBeeComm::LENLSB;
				break;

			case CXBeeComm::LENLSB:
				RXLength += b; // LSB
				if (RXLength > (100 + 11)) RXPhase = CXBeeComm::START; // packet too large, reset
				RXPhase = CXBeeComm::FRAMETYPE;
				break;

			case CXBeeComm::FRAMETYPE:
				RXFrameType = b;
				if (RXFrameType == 0x80)
				{              
					// DATA
					RXCounter = 0; // reset counter
					RXChkSum += b;
					RXPhase = CXBeeComm::SOURCEADDR; 
				}
				else if(RXFrameType == 0x88)
				{
					// AT Command response
					RXCounter = 0; // reset counter
					RXChkSum += b;
					RXLength += 10; // "length" hacked to reuse function
					RXPhase = CXBeeComm::DATA;
				}
				else RXPhase = CXBeeComm::START; // ignore all other packets!
				break;

			case CXBeeComm::SOURCEADDR:
				RXSourceAddr[RXCounter] = b;
				RXCounter++;
				RXChkSum += b;
				if( RXCounter == 8 ) RXPhase = CXBeeComm::RSSI;
				break;

			case CXBeeComm::RSSI:
				RXRSSI = b;
				RXChkSum += b;
				RXPhase = CXBeeComm::OPTIONS;                        
				break;

			case CXBeeComm::OPTIONS:
				RXOptions = b;
				RXCounter = 0; // reset counter
				RXChkSum += b;
				RXPhase = CXBeeComm::DATA;
				break;

			case CXBeeComm::DATA:
				RXData[RXCounter] = b;
				RXCounter++;
				RXChkSum += b;
				if (RXCounter == RXLength - 11) RXPhase = CXBeeComm::CHECKSUM;
				break;

			case CXBeeComm::CHECKSUM:
				if ( (RXChkSum + b) == 0xFF )
				{
					// OK
					NewPacketCallback(RXData, RXLength - 11, RXRSSI, RXFrameType);
				}
				else
				{
					// checksum error!
				}
				RXPhase = CXBeeComm::START; // wait for new
				break;
		}
	}
}

// max data size is 100!!!
int CXBeeComm::GenerateTXPacket(BYTE* data, int dataLen, BYTE* packet)
{	
    // assemble header
    packet[0] = 0x7E;
    int length = dataLen + 11; // 11 - header length 
    packet[1] = (BYTE)(length / 256);
    packet[2] = (BYTE)(length % 256);
    packet[3] = 0x00; // 64-bit TX
    packet[4] = 0x00; // FrameID - disable response frame!!!
	memcpy(&packet[5], DestinationAddr, 8); // copy destination address
    packet[13] = 0x00; // use 0x01 to disable ACK, maybe it will increase throughtput?!?!?
	memcpy(&packet[14], data, dataLen); // copy data
    packet[dataLen + 14] = CalculateCheckSum(packet, dataLen);

	return dataLen+15; // return total length
}

int CXBeeComm::GenerateATPacket(BYTE* command, BYTE param, BYTE* packet)
{
	// assemble header
	packet[0] = 0x7E;
	int length = 2+2+1; // fixed length (lengthX2 + commandX2 + param)
	packet[1] = (BYTE)(length / 256);
	packet[2] = (BYTE)(length % 256);
	packet[3] = 0x08; // AT Commands
	packet[4] = 0x01; // FrameID - wait response
	memcpy(&packet[5], command, 2); // two commmand bytes (e.g. "ED")
	packet[7] = param; // parameters
	packet[8] = CalculateCheckSum(packet, length-11); // "length" hacked to reuse function

	return length + 4; // return total length
}

BYTE CXBeeComm::CalculateCheckSum(BYTE* packet, int dataLen)
{
	unsigned int sum = 0;
    for (int i = 3; i != dataLen + 14; i++)
    {
        sum += packet[i];
    }

    return (BYTE)(0xFF - (BYTE)sum);
}
