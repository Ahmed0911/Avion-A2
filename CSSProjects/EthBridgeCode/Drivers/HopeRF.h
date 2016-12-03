/*
 * HopeRF.h
 *
 *  Created on: Jul 6, 2015
 *      Author: User
 */

#ifndef HOPERF_H_
#define HOPERF_H_

#include "../Fifo.h"
#include "Timer.h"

class HopeRF
{
public:
	bool Init();
	void Reset();

	bool Write(BYTE* buffer, BYTE size); // Write To FIFO Buffer, Queue for send
	int Read(BYTE* buffer); // Get Received Data, returns 0 if not available

	void IntHandler(void);
	int ReadRSSI(void);

	void ChangeFrequency(unsigned int frequency);

private:
	bool WriteFromBuffer();
	void InitRX();

	// Low Level
	void ReadBytes(unsigned char address, unsigned char* buffer, int count);
	void WriteBytes(unsigned char address, unsigned char* buffer, int count);
	void ClearSSIFIFO();
	unsigned char ReadReg(unsigned char address);
	void WriteReg(unsigned char address, unsigned char value);

	// RF Modem Internal Function
	void SetupChannel(unsigned int frequency, unsigned char bandwidth, unsigned char coding, unsigned char spreadingFactor, int preambleLength );
	void SetOutputPower(unsigned char power_dB);
	void SetMode(int mode);
	int GetModemStatus(void);

private:
	// TX Stuff
	Fifo m_TXFifo;
	Timer m_txTimer;
	float m_txTimeToSend;
	bool m_TXInProgress;
	int m_TXCount;

	// RX Stuff
	Fifo m_RXFifo;
	int m_RXCRCErrors;
	int m_RXPacketSNR;
	int m_INTCounter;

public:
	int PacketRSSI;
	int ReceivedFrames;


};

// Registers
#define HOPERF_REG_LR_FIFO			0x00
#define HOPERF_REG_LR_OPMODE		0x01
#define HOPERF_REG_LR_FRFMSB		0x06
#define HOPERF_REG_LR_PACONFIG		0x09
#define HOPERF_REG_LR_OCP          	0x0B
#define HOPERF_REG_LR_FIFOADDRPTR	0x0D
#define HOPERF_REG_LR_FIFOTXBASEADDR	0x0E
#define HOPERF_REG_LR_FIFORXBASEADDR	0x0F
#define HOPERF_REG_LR_FIFORXCURRENTADDR	0x10
#define HOPERF_REG_LR_IRQFLAGSMASK	0x11
#define HOPERF_REG_LR_IRQFLAGS      0x12
#define HOPERF_REG_LR_NBRXBYTES		0x13
#define HOPERF_REG_LR_MODEMSTAT     0x18
#define HOPERF_REG_LR_PKTSNRVALUE  	0x19
#define HOPERF_REG_LR_PKTRSSIVALUE  0x1A
#define HOPERF_REG_LR_RSSIVALUE		0x1B
#define HOPERF_REG_LR_MODEMCONFIG1  0x1D
#define HOPERF_REG_LR_MODEMCONFIG2  0x1E
#define HOPERF_REG_LR_PREAMBLEMSB	0x20
#define HOPERF_REG_LR_PREAMBLELSB	0x21
#define HOPERF_REG_LR_PAYLOADLENGTH	0x22
#define HOPERF_REG_LR_HOPPERIOD		0x24
#define HOPERF_REG_LR_MODEMCONFIG3	0x26
#define HOPERF_REG_LR_DIOMAPPING1	0x40
#define HOPERF_REG_LR_DIOMAPPING2	0x41
#define HOPERF_REG_LR_VERSION		0x42
#define HOPERF_REG_LR_PADAC         0x4D



// Constants
#define HOPERF_BW_7_81_KHZ               0x00
#define HOPERF_BW_10_41_KHZ              0x10
#define HOPERF_BW_15_62_KHZ              0x20
#define HOPERF_BW_20_83_KHZ              0x30
#define HOPERF_BW_31_25_KHZ              0x40
#define HOPERF_BW_41_66_KHZ              0x50
#define HOPERF_BW_62_50_KHZ              0x60
#define HOPERF_BW_125_KHZ                0x70 /* Default */
#define HOPERF_BW_250_KHZ                0x80
#define HOPERF_BW_500_KHZ                0x90

#define HOPERF_CODINGRATE_4_5            0x02
#define HOPERF_CODINGRATE_4_6            0x04 /* Default */
#define HOPERF_CODINGRATE_4_7            0x06
#define HOPERF_CODINGRATE_4_8            0x08

#define	HOPERF_OPMODE_SLEEP                           0x00
#define HOPERF_OPMODE_STANDBY                         0x01
#define HOPERF_OPMODE_SYNTHESIZER_TX                  0x02
#define HOPERF_OPMODE_TRANSMITTER                     0x03
#define HOPERF_OPMODE_SYNTHESIZER_RX                  0x04
#define HOPERF_OPMODE_RECEIVER                        0x05
#define HOPERF_OPMODE_RECEIVER_SINGLE                 0x06
#define HOPERF_OPMODE_CAD                             0x07

#define HOPERF_IRQFLAGS_RXTIMEOUT                     0x80
#define HOPERF_IRQFLAGS_RXDONE                        0x40
#define HOPERF_IRQFLAGS_PAYLOADCRCERROR               0x20
#define HOPERF_IRQFLAGS_VALIDHEADER                   0x10
#define HOPERF_IRQFLAGS_TXDONE                        0x08
#define HOPERF_IRQFLAGS_CADDONE                       0x04
#define HOPERF_IRQFLAGS_FHSSCHANGEDCHANNEL            0x02
#define HOPERF_IRQFLAGS_CADDETECTED                   0x01

#define HOPERF_DIOMAPPING1_DIO0_00_RXDONE             0x00 /* RxDone */
#define HOPERF_DIOMAPPING1_DIO0_01_TXDONE             0x40 /* TxDone */

#define HOPERF_MODEMSTAT_MODEM_CLEAR				  0x10
#define HOPERF_MODEMSTAT_HEADER_VALID				  0x08
#define HOPERF_MODEMSTAT_RX_ONGOING					  0x04
#define HOPERF_MODEMSTAT_SIG_SYNC					  0x02
#define HOPERF_MODEMSTAT_SIG_DETECTED				  0x01

#endif /* HOPERF_H_ */
