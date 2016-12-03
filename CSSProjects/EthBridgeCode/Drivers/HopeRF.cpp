/*
 * HopeRF.cpp
 *
 *  Created on: Jul 6, 2015
 *      Author: User
 */

#include "HopeRF.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/ssi.h"

extern uint32_t g_ui32SysClock;

#define SELECT()  GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, 0 )
#define DESELECT()  GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_PIN_4 )

// SPI
// ---------
// 10MHz max
// Freescale SPI
// SPO = 0,SPH = 0 -> FRF_MOTO_MODE_0
// SSI1
// PINS:
// PB5 - CLK
// PE4 - MOSI
// PE5 - MISO
// PQ4 - CS
// PN2 - DIO0
// PN1 - DIO1
// PN0 - DIO2
// PP4 - DIO3
// PP5 - DIO4
// PP3 - DIO5
// PP2 - RESET
bool HopeRF::Init()
{
	m_INTCounter = 0;
	m_TXInProgress = false;
	m_TXCount = 0;
	ReceivedFrames = 0;
	PacketRSSI = -150;

	// Enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	// SPI Pins
	GPIOPinConfigure(GPIO_PB5_SSI1CLK);
	GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
	GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
	GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5);
	GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	// Chip Select Pins - CS
	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_PIN_4 ); // default to 1
	GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_4);

	// RESET PIN
	GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_2, GPIO_PIN_2 );
	GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_2);

	// Set INPUT PINs
	GPIOPinTypeGPIOInput(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0 );
	GPIOPinTypeGPIOInput(GPIO_PORTP_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_3 );

	// INT Pins - DIO0 Only->PN2
	GPIOIntEnable(GPIO_PORTN_BASE, GPIO_PIN_2); // Set AS INT Source!!!
	GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_PIN_2, GPIO_RISING_EDGE);
	IntEnable(INT_GPION);

	// Configure
	SSIConfigSetExpClk(SSI1_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 10000000, 8);
	// Enable
	SSIEnable(SSI1_BASE);

	// Clear fifo
	ClearSSIFIFO();

	// Reset
	Reset();

	// check IDs
	BYTE ID = ReadReg(HOPERF_REG_LR_VERSION); // Reg Version
	if( ID != 0x12 ) return false;

	// Set LoRA Mode ON
	WriteReg(HOPERF_REG_LR_OPMODE, 0x08); // sleep mode (0X08 - LFMODE on), LORA OFF
	SysCtlDelay(g_ui32SysClock/3/10000); // 100 us delay ???
	SetMode(HOPERF_OPMODE_SLEEP); // sleep mode with LORA ON

	// Set Frequency
	SetupChannel(434000000, HOPERF_BW_500_KHZ, HOPERF_CODINGRATE_4_5, 7, 12); // Link: 135dB, 100 Bytes frame: 60 ms , 22000 bps (10 km?)
	//SetupChannel(434000000, HOPERF_BW_125_KHZ, HOPERF_CODINGRATE_4_5, 7, 12); // Link: 141dB, 100 Bytes frame: 235 ms , 5500 bps
	//SetupChannel(434000000, HOPERF_BW_500_KHZ, HOPERF_CODINGRATE_4_5, 12, 12);// Link: 149dB, 100 Bytes frame: 1020 ms , 1200 bps
	//SetupChannel(434000000, HOPERF_BW_62_50_KHZ, HOPERF_CODINGRATE_4_5, 12, 12); // Link: 158dB, 100 Bytes frame:8150 ms , 150 bps (100 km?)

	// Init LoRa
	SetOutputPower(17); // 17 dBm
	WriteReg(HOPERF_REG_LR_DIOMAPPING1, 0x00);	//RegDioMapping1: DIO0 - RxDone(*), DIO1 - TxTimeout, DIO2 - FhssChange, DIO3 - CadDone
	WriteReg(HOPERF_REG_LR_DIOMAPPING2, 0x00);	//RegDioMapping2: DIO4 - CadDetected, DIO5 - ModeReady

	// Go to Standby
	SetMode(HOPERF_OPMODE_STANDBY);

	InitRX();

	return true;
}

// reset chip!
void HopeRF::Reset()
{
	GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_2, GPIO_PIN_0 ); // reset
	SysCtlDelay(g_ui32SysClock/3/1000); // 1 ms delay
	GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_2, GPIO_PIN_2 );
	SysCtlDelay(g_ui32SysClock/3/100); // 10 ms delay
}

// Queue To FIFO To Send
bool HopeRF::Write(BYTE* buffer, BYTE size)
{
	IntMasterDisable();

	bool success = false;

	// put to buffer
	int requiredSize = size+1; // buffer + [size]
	if( m_TXFifo.HasSpace(requiredSize) )
	{
		m_TXFifo.Push(size);
		for(int i=0; i!=size; i++)
		{
			m_TXFifo.Push(buffer[i]);
		}
		success = true;
	}

	// Trigger Write
	bool TXRXInProgress = false;
	if( m_TXInProgress ) TXRXInProgress = true;
	if( GetModemStatus() & HOPERF_MODEMSTAT_SIG_DETECTED ) TXRXInProgress = true; // Check RX in progress
	if( TXRXInProgress == false ) WriteFromBuffer();

	IntMasterEnable();

	return success;
}

// Get RX Data From FIFO
int HopeRF::Read(BYTE* buffer)
{
	IntMasterDisable();

	BYTE size = 0;
	if( m_RXFifo.IsEmpty() == false)
{
		m_RXFifo.Pop(size); // get size of packet from FIFO to send
		for(int i=0; i!= size; i++) m_RXFifo.Pop(buffer[i]); // get Data from FIFO
}

	IntMasterEnable();

	return size;
}

bool HopeRF::WriteFromBuffer()
{
	bool success = false;

	BYTE size;
	BYTE buffer[255]; // MAX Packet SIZE = 255!!!
	m_TXFifo.Pop(size); // get size of packet from FIFO to send
	for(int i=0; i!= size; i++) m_TXFifo.Pop(buffer[i]); // get Data from FIFO

	// Go to STANDBY MODE
	SetMode(HOPERF_OPMODE_STANDBY);
	// Set Hop Period to zero (NO HOPING)
	WriteReg(HOPERF_REG_LR_HOPPERIOD, 0);
	// Set IRQs masks (set to 1 to disable)
	BYTE irqMask = HOPERF_IRQFLAGS_RXTIMEOUT |
					HOPERF_IRQFLAGS_RXDONE |
					HOPERF_IRQFLAGS_PAYLOADCRCERROR |
					HOPERF_IRQFLAGS_VALIDHEADER |
					//HOPERF_IRQFLAGS_TXDONE |
					HOPERF_IRQFLAGS_CADDONE |
					HOPERF_IRQFLAGS_FHSSCHANGEDCHANNEL |
					HOPERF_IRQFLAGS_CADDETECTED;
	WriteReg(HOPERF_REG_LR_IRQFLAGSMASK, irqMask);
	// Payload Length
	WriteReg(HOPERF_REG_LR_PAYLOADLENGTH, size);
	BYTE txAddr = 0x00; // Full buffer used for Tx
	WriteReg(HOPERF_REG_LR_FIFOTXBASEADDR, txAddr);
	WriteReg(HOPERF_REG_LR_FIFOADDRPTR, txAddr);

	// Write Data to FIFO
	WriteBytes(HOPERF_REG_LR_FIFO, buffer, size );

	// DIO Mapping for TX
	WriteReg(HOPERF_REG_LR_DIOMAPPING1, 0x00 + HOPERF_DIOMAPPING1_DIO0_01_TXDONE);	//RegDioMapping1: DIO0 - TxDone, DIO1 - RxTimeout, DIO2 - FhssChange, DIO3 - CadDone
	WriteReg(HOPERF_REG_LR_DIOMAPPING2, 0x00);	//RegDioMapping2: DIO4 - CadDetected, DIO5 - ModeReady

	// Start TX
	SetMode( HOPERF_OPMODE_TRANSMITTER );
	m_txTimer.Start(); // DEBUG->TX Timer
	m_TXInProgress = true;

	return success;
}

void HopeRF::IntHandler(void)
{
	unsigned int ulStatus = GPIOIntStatus(GPIO_PORTN_BASE, true);

	// Clear all the pin interrupts that are set
	GPIOIntClear(GPIO_PORTN_BASE, ulStatus);
	if(ulStatus & GPIO_PIN_2) // DIO0 INT
	{
		// Process INT
		if( m_TXInProgress )
		{
			// Update TX Timer
			m_txTimeToSend = m_txTimer.GetUS();
			m_TXCount++;

			// Clear Irq
			WriteReg( HOPERF_REG_LR_IRQFLAGS, HOPERF_IRQFLAGS_TXDONE  );

			// Return to standby
			SetMode(HOPERF_OPMODE_STANDBY);
			m_TXInProgress = false;

			// More to Send?
			if( !m_TXFifo.IsEmpty() )
			{
				WriteFromBuffer();
			}
			else
			{
				// Return to RX Mode!
				InitRX();
			}
		}
		else
		{
			// must be RX in progress
			BYTE size = 0;
			BYTE buffer[255]; // MAX Packet SIZE = 255!!!

			// Clear Irq
			WriteReg( HOPERF_REG_LR_IRQFLAGS, HOPERF_IRQFLAGS_RXDONE  );

			// Check Payload CRC
			BYTE irqFlags = ReadReg(HOPERF_REG_LR_IRQFLAGS);
			if( ( irqFlags & HOPERF_IRQFLAGS_PAYLOADCRCERROR ) == HOPERF_IRQFLAGS_PAYLOADCRCERROR )
			{
				// Clear Irq
				WriteReg( HOPERF_REG_LR_IRQFLAGS, HOPERF_IRQFLAGS_PAYLOADCRCERROR  );
				// ---->ignore packet
				m_RXCRCErrors++;
			}
			else
			{
				// Packet SNR
				BYTE rxSnrEstimate = ReadReg(HOPERF_REG_LR_PKTSNRVALUE);
				if( rxSnrEstimate & 0x80 ) // The SNR sign bit is 1
				{
					m_RXPacketSNR = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2; // Invert and divide by 4
					m_RXPacketSNR = -m_RXPacketSNR;
				}
				else
				{
					m_RXPacketSNR = ( rxSnrEstimate & 0xFF ) >> 2; // Divide by 4
				}

				// Packet RSSI
				BYTE rxRSSI = ReadReg(HOPERF_REG_LR_PKTRSSIVALUE);
				if( m_RXPacketSNR < 0 )
				{
					PacketRSSI = -164 + rxRSSI + m_RXPacketSNR;
				}
				else
				{
					PacketRSSI = -164 + rxRSSI;
				}

				BYTE currAddr = ReadReg( HOPERF_REG_LR_FIFORXCURRENTADDR );
				size = ReadReg(HOPERF_REG_LR_NBRXBYTES);
				WriteReg(HOPERF_REG_LR_FIFOADDRPTR, currAddr);
				ReadBytes(HOPERF_REG_LR_FIFO, buffer, size); // get packet

				ReceivedFrames++;

				// TODO Immediate Mode/callback?

				// Put to FIFO buffer
				int requiredSize = size+1; // buffer + [size]
				if( m_RXFifo.HasSpace(requiredSize) )
				{
					m_RXFifo.Push(size);
					for(int i=0; i!=size; i++)
					{
						m_RXFifo.Push(buffer[i]);
					}
				}
			}

			// Data to Send?
			if( !m_TXFifo.IsEmpty() )
			{
				WriteFromBuffer();
			}
		}

		m_INTCounter++;
	}
}

void HopeRF::InitRX()
{
	// Go to STANDBY MODE
	SetMode(HOPERF_OPMODE_STANDBY);
	// Set Hop Period to zero (NO HOPING)
	WriteReg(HOPERF_REG_LR_HOPPERIOD, 0);
	// Set IRQs masks (set to 1 to disable)
	BYTE irqMask = HOPERF_IRQFLAGS_RXTIMEOUT |
					//HOPERF_IRQFLAGS_RXDONE |
					//HOPERF_IRQFLAGS_PAYLOADCRCERROR |
					HOPERF_IRQFLAGS_VALIDHEADER |
					HOPERF_IRQFLAGS_TXDONE |
					HOPERF_IRQFLAGS_CADDONE |
					HOPERF_IRQFLAGS_FHSSCHANGEDCHANNEL |
					HOPERF_IRQFLAGS_CADDETECTED;
	WriteReg(HOPERF_REG_LR_IRQFLAGSMASK, irqMask);

	// Payload Length
	BYTE rxAddr = 0x00; // Full buffer used for Rx
	WriteReg(HOPERF_REG_LR_FIFORXBASEADDR, rxAddr);
	WriteReg(HOPERF_REG_LR_FIFOADDRPTR, rxAddr);

	// DIO Mapping for RX
	WriteReg(HOPERF_REG_LR_DIOMAPPING1, 0x00 + HOPERF_DIOMAPPING1_DIO0_00_RXDONE);	//RegDioMapping1: DIO0 - RxDone, DIO1 - RxTimeout, DIO2 - FhssChange, DIO3 - CadDone
	WriteReg(HOPERF_REG_LR_DIOMAPPING2, 0x00);	//RegDioMapping2: DIO4 - CadDetected, DIO5 - ModeReady

	// Start RX
	SetMode( HOPERF_OPMODE_RECEIVER );
}

void HopeRF::SetMode(int mode)
{
	WriteReg(HOPERF_REG_LR_OPMODE, 0x88 + mode); // LoRa Mode (0x80) + LF Mode Regs (0x08) + MODE
}

// Returns RSSI
int HopeRF::ReadRSSI(void)
{
	 // Reads the RSSI value
	BYTE val = ReadReg(HOPERF_REG_LR_RSSIVALUE);

	int rssi = -164 + (int)val;

	return rssi;
}

// Returns LoRa RX Status
int HopeRF::GetModemStatus(void)
{
	// Reads the Modem Status
	BYTE val = ReadReg(HOPERF_REG_LR_MODEMSTAT);
	val = val & 0x1F; // Remove RX Coding Rate

	return val;
}

// Helpers
// Freq bands:
// 1. 137000000 - 175000000
// 2. 410000000 - 525000000
// Spreading 6 N/A Yet!!! Must set header to implicit mode + DetectinThreshold + DetectOptimize!
void HopeRF::SetupChannel(unsigned int frequency, unsigned char bandwidth, unsigned char coding, unsigned char spreadingFactor, int preambleLength )
{
	BYTE buf[20];

	// set frequency
	uint32_t freq = ( uint32_t )( ( double )frequency / ( double )61.03515625 );
	buf[0] = ( BYTE )( ( freq >> 16 ) & 0xFF ); // MSB
	buf[1] = ( BYTE )( ( freq >> 8 ) & 0xFF ); // MID
	buf[2] = ( BYTE )( freq & 0xFF ); // LSB
	WriteBytes(HOPERF_REG_LR_FRFMSB, buf, 3);

	// Set Bandwitdth
	BYTE val = bandwidth + coding + 0x00; // 0x00 - explicit header mode
	WriteReg(HOPERF_REG_LR_MODEMCONFIG1, val);

	// Set Spreading
	if( spreadingFactor < 7 ) spreadingFactor = 7;
	if( spreadingFactor > 12 ) spreadingFactor = 12;
	val = (spreadingFactor << 4) + 0x04; // 0x04 - CRC ON
	WriteReg(HOPERF_REG_LR_MODEMCONFIG2, val);

	// Low Data Rate Optimize
	val = 0x08; // 0x08 - low data rate opt.
	WriteReg(HOPERF_REG_LR_MODEMCONFIG3, val);

	// Preamble Length
	buf[0] = ( BYTE )( ( preambleLength >> 8 ) & 0xFF ); // MSB
	buf[1] = ( BYTE )( preambleLength & 0xFF ); // LSB
	WriteBytes (HOPERF_REG_LR_PREAMBLEMSB, buf, 2);
}

// Min 2 dBm
// Max 17dBm
// +Special high power 20dBm
void HopeRF::SetOutputPower(unsigned char power_dB)
{
	if( power_dB < 2) power_dB = 2;
	if( power_dB > 17 ) power_dB = 20; // high power +20dB

	// Output Power
	BYTE val = 0x80 + 0x70; // 0x80 - PA_BOOST pin, 0x70 - max power(not used for PA_BOOST mode)
	if( power_dB < 20 ) val += (power_dB - 2);
	else val += 0x0F; // max power
	WriteReg(HOPERF_REG_LR_PACONFIG, val);

	// PA BOOST PIN
	if( power_dB < 20 ) WriteReg(HOPERF_REG_LR_PADAC, 0x84);
	else WriteReg(HOPERF_REG_LR_PADAC, 0x87); // +20dBm

	// Disable OCP (use for high power!)
	//WriteReg(HOPERF_REG_LR_OCP, 0x0B);
}

// Freq bands:
// 1. 137000000 - 175000000
// 2. 410000000 - 525000000
void HopeRF::ChangeFrequency(unsigned int frequency)
{
	BYTE buf[20];

	SetMode(HOPERF_OPMODE_STANDBY);

	// set frequency
	uint32_t freq = ( uint32_t )( ( double )frequency / ( double )61.03515625 );
	buf[0] = ( BYTE )( ( freq >> 16 ) & 0xFF ); // MSB
	buf[1] = ( BYTE )( ( freq >> 8 ) & 0xFF ); // MID
	buf[2] = ( BYTE )( freq & 0xFF ); // LSB
	WriteBytes(HOPERF_REG_LR_FRFMSB, buf, 3);

	SetMode( HOPERF_OPMODE_RECEIVER );
}

// SPI STUFF
void HopeRF::ClearSSIFIFO()
{
	// Clear FIFO
	uint32_t data;
	while(SSIDataGetNonBlocking(SSI1_BASE, &data));
}

// FIFO is 8 words wide!
void HopeRF::ReadBytes(unsigned char address, unsigned char* buffer, int count)
{
	uint32_t data;
	SELECT();

	SSIDataPut(SSI1_BASE, address + 0x00); // 0x00 - READ FLAG
	while(SSIBusy(SSI1_BASE));
	SSIDataGet(SSI1_BASE, &data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI1_BASE, 0x00); // READ
		while(SSIBusy(SSI1_BASE));
		SSIDataGet(SSI1_BASE, &data);
		buffer[i] = (data&0x00FF);
	}

	DESELECT();
}

void HopeRF::WriteBytes(unsigned char address, unsigned char* buffer, int count)
{
	uint32_t data;
	SELECT();

	SSIDataPut(SSI1_BASE, address + 0x80); // 0x80 - WRITE FLAG
	while(SSIBusy(SSI1_BASE));
	SSIDataGet(SSI1_BASE, &data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI1_BASE, buffer[i]); // WRITE
		while(SSIBusy(SSI1_BASE));
		SSIDataGet(SSI1_BASE, &data);
	}

	DESELECT();
}

// helpers
unsigned char HopeRF::ReadReg(unsigned char address)
{
	unsigned char data = 0;
	ReadBytes(address, &data, 1);

	return data;
}

void HopeRF::WriteReg(unsigned char address, unsigned char value)
{
	WriteBytes(address, &value, 1);
}
