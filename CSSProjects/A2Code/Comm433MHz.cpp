/*
 * Comm433MHz.cpp
 *
 *  Created on: Feb 12, 2017
 *      Author: Ivan
 */

#include "Comm433MHz.h"
#include "CRC32.h"

extern class CRC32 crc;

extern void ProcessCommand(int cmd, unsigned char* data, int dataSize);

Comm433MHz::Comm433MHz()
{
    MsgReceivedOK = 0;
    CrcErrors = 0;
    HeaderFails = 0;
}

Comm433MHz::~Comm433MHz()
{
    // TODO Auto-generated destructor stub
}

void Comm433MHz::NewRXPacket(BYTE* data, int dataLen)
{
    // RX Parser
    for (int i = 0; i != dataLen; i++)
    {
        BYTE b = data[i];
        switch (RXPhase)
        {
            // HEADER
            case Comm433MHz::HDR_FF:
                if (b == 0xFF)
                {
                    RXPhase = Comm433MHz::HDR_FE; // wait for start
                }
                break;

            case Comm433MHz::HDR_FE:
                if (b == 0xFE)
                {
                    RXPhase = Comm433MHz::HDR_A5; // wait for start
                }
                else
                {
                    RXPhase = Comm433MHz::HDR_FF; // reset
                    HeaderFails++;
                }
                break;

            case Comm433MHz::HDR_A5:
                if (b == 0xA5)
                {
                    RXPhase = Comm433MHz::HDR_5A; // wait for start
                }
                else
                {
                    RXPhase = Comm433MHz::HDR_FF; // reset
                    HeaderFails++;
                }
                break;

            case Comm433MHz::HDR_5A:
                if (b == 0x5A)
                {
                    RXPhase = Comm433MHz::TYPE; // wait for start
                }
                else
                {
                    RXPhase = Comm433MHz::HDR_FF; // reset
                    HeaderFails++;
                }
                break;

            case Comm433MHz::TYPE:
                Type = b;
                RXPhase = Comm433MHz::LEN;
                break;

            case Comm433MHz::LEN:
                Len = b;
                DataIndex = 0; // reset data counter
                RXPhase = Comm433MHz::DATA;
                break;


            case Comm433MHz::DATA:
                Data[DataIndex] = b;
                DataIndex++;
                if (DataIndex >= Len) RXPhase = Comm433MHz::CRC_A;
                break;

            case Comm433MHz::CRC_A:
                CRC = (unsigned int)(b << 24);
                RXPhase = Comm433MHz::CRC_B;
                break;

            case Comm433MHz::CRC_B:
                CRC += (unsigned int)(b << 16);
                RXPhase = Comm433MHz::CRC_C;
                break;

            case Comm433MHz::CRC_C:
                CRC += (unsigned int)(b << 8);
                RXPhase = Comm433MHz::CRC_D;
                break;

            case Comm433MHz::CRC_D:
                CRC += b;
                // check CRC, process data!!
                unsigned int calculatedCRC = crc.CalculateCRC32(Data, Len);
                if(calculatedCRC == CRC)
                {
                    // message OK, process!
                    MsgReceivedOK++;

                    ProcessMessage(Type, Data, Len);
                }
                else
                {
                    // CRC Failed
                    CrcErrors++;
                }

                RXPhase = Comm433MHz::HDR_FF;
                break;
        }

    }
}

void Comm433MHz::ProcessMessage(BYTE type, BYTE* data, BYTE len)
{
    ProcessCommand(type, data, len);
}

int Comm433MHz::GenerateTXPacket(BYTE Type, BYTE* Data, BYTE Len, BYTE* OutputPacket)
{
    // assemble header
    OutputPacket[0] = 0xFF; // Header
    OutputPacket[1] = 0xFE; // Header
    OutputPacket[2] = 0xA5; // Header
    OutputPacket[3] = 0x5A; // Header

    OutputPacket[4] = Type; // Type
    OutputPacket[5] = Len; // Data Length
    for(int i=0;i!=Len; i++)
    {
        OutputPacket[6 + i] = Data[i];
    }

    // CRC
    unsigned int calculatedCRC = crc.CalculateCRC32(Data, Len);
    OutputPacket[6 + Len] = (BYTE)(calculatedCRC >> 24);
    OutputPacket[6 + Len+1] = (BYTE)(calculatedCRC >> 16);
    OutputPacket[6 + Len+2] = (BYTE)(calculatedCRC >> 8);
    OutputPacket[6 + Len+3] = (BYTE)(calculatedCRC);

    return 6 + Len + 4; // return total length (4xHEDER + TYPE + LEN + 4xCHKSUM)
}
