#include "EtherDriver.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>
#include <driverlib/ethernet.h>
#include <driverlib/interrupt.h>
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include <string.h>
#include <stdio.h>
#include "../Datafile.h"
#include "../Timer.h"

extern SDataFile datafile;

extern void ProcessCommand(int cmd, unsigned char* data, int dataSize);

void EtherUDPRecv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	if (p != NULL)
	{
		((EtherDriver*)arg)->DataReceived(p, addr, port);
		pbuf_free(p); // free received buffer
	}
}

void EtherDriver::Init()
{
	// enable Ethernet LEDs
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinConfigure(GPIO_PF3_LED0);
	GPIOPinConfigure(GPIO_PF2_LED1);
	GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

	// Initialize the Ethernet controller for operation
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
	SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);
	EthernetInitExpClk(ETH_BASE, SysCtlClockGet());
	IntPrioritySet(INT_ETH, (1 << 5));

	// Get MAC Address, from flash!
	unsigned char pucMACAddress[6];
	pucMACAddress[0] = 0x10;
	pucMACAddress[1] = 0x22;
	pucMACAddress[2] = 0x33;
	pucMACAddress[3] = 0x33;
	pucMACAddress[4] = 0x23;
	pucMACAddress[5] = 0xAA;


	// lwIP Ethernet
	// Set IP
	lwIPInit(pucMACAddress, inet_addr("200.1.0.10"), inet_addr("0.255.255.255"), inet_addr("1.1.0.10"), IPADDR_USE_STATIC);

	// Setup the device locator service. (can be found by "finder.exe")
	LocatorInit();
	LocatorMACAddrSet(pucMACAddress);
	LocatorAppTitleSet("Logger Board");
	// init lwIP
	m_udpPcb =  udp_new();
	udp_bind(m_udpPcb, IP_ADDR_ANY, ETHPORT);
	udp_recv(m_udpPcb, EtherUDPRecv, this );

	DestinationPort = 0;

	// RAW Ethernet
	// Configure the Ethernet controller for normal operation
	// Enable TX Duplex Mode
	// Enable TX Padding
	//EthernetConfigSet(ETH_BASE, (ETH_CFG_TX_DPLXEN | ETH_CFG_TX_PADEN));
	//EthernetMACAddrSet(ETH_BASE, pucMACAddress); // Set MAC
	//EthernetEnable(ETH_BASE); // Enable the Ethernet controller
}

void EtherDriver::DataReceived(pbuf *p, ip_addr *addr, u16_t port)
{
	// process incoming data, send response back to receiver
	// check magic codes
	bool ok = IsPacketValid(p);

	if( ok )
	{
		// process packet
		unsigned char* data = (unsigned char*)p->payload;
		int dataSize = p->len-3;
		datafile.ReceivedPackets++;

		switch( data[10] )
		{
			case 0x10: // PING, send all data
			{
				// Do nothing

				break;
			}

			default:
				ProcessCommand(data[2], &data[3], dataSize);
				break;
		}
	}
}

bool EtherDriver::IsPacketValid(pbuf* p)
{
	bool ok = true;
	if( p->len < 3 ) return false;
	unsigned char* data = (unsigned char*)p->payload;
	if( data[0] != 0x42) ok = false;
	if( data[1] != 0x24) ok = false;

	return ok;
}

bool EtherDriver::SendPacket(char type, char* txBuff, int size)
{
	if( DestinationPort == 0) return false; // target not set yet, needs at least one PING!

	Timer timer; timer.Init();
	SendPacket(type, txBuff, size, &DestinationAddr, DestinationPort);
	datafile.SendPacketTimeMS = timer.GetMS();
	return true;
}

void EtherDriver::SendPacket(char type, char* txBuff, int size, ip_addr *destination, u16_t destinationPort)
{
	pbuf *pkt = pbuf_alloc(PBUF_TRANSPORT,size+3,PBUF_RAM);

	// Add Header: magic codes+type
	char* destBuffer = (char*)pkt->payload;
	destBuffer[0] = 0x42;
	destBuffer[1] = 0x24;
	destBuffer[2] = type;
	memcpy( (void*)&destBuffer[3], (void*)txBuff, size);
	udp_sendto(m_udpPcb, pkt, destination, destinationPort);
	pbuf_free(pkt); //De-allocate packet buffer

	datafile.SentPackets++;
}


void EtherDriver::Process(int ms)
{
	// call lwIPTimer
	lwIPTimer(ms);

	// do all additional COMM here! (must be called from same context/interrupt as lwIPTImer!)
}
//*****************************************************************************
//
// Called from lwIP interrupt.
// Set by HOST_TMR_INTERVAL in lwipopts.h
//
//*****************************************************************************
extern "C" void lwIPHostTimerHandler(void)
{
    // XXX: add something here
}
