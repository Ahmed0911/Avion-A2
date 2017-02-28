/*
 * Comm433MHz.h
 *
 *  Created on: Feb 12, 2017
 *      Author: Ivan
 */

#ifndef COMM433MHZ_H_
#define COMM433MHZ_H_

typedef unsigned char BYTE;

class Comm433MHz
{
public:
    Comm433MHz();
    virtual ~Comm433MHz();

public:
    void ProcessMessage(BYTE type, BYTE* data, BYTE len);
    int GenerateTXPacket(BYTE Type, BYTE* Data, BYTE Len, BYTE* OutputPacket);
    void NewRXPacket(BYTE* data, int dataLen);

public:
     enum ERXPhase { HDR_FF, HDR_FE, HDR_A5, HDR_5A, TYPE, LEN, DATA, CRC_A, CRC_B, CRC_C, CRC_D };
     ERXPhase RXPhase;

     BYTE Type;
     BYTE Len;
     BYTE Data[255];
     int DataIndex;
     unsigned int CRC;

     // counters
     int MsgReceivedOK;
     int CrcErrors;
     int HeaderFails;
};

#endif /* COMM433MHZ_H_ */
