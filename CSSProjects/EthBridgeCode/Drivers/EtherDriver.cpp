#include "EtherDriver.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_emac.h"
#include "driverlib/gpio.h"
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/emac.h>
#include <driverlib/interrupt.h>

#include "utils/locator.h"
#include "utils/lwiplib.h"
#include <lwip/sockets.h>
#include <lwip/inet.h>

extern uint32_t g_ui32SysClock;
extern void ProcessCommand(int cmd, unsigned char* data, int dataSize);

void EtherUDPRecv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	if (p != NULL)
	{
		((EtherDriver*)arg)->DataReceived(p, addr, port);
		pbuf_free(p); // free received buffer
	}
}

// PINs:
// PK5 - LED2
// PK6 - LED1
void EtherDriver::Init()
{
	// enable Ethernet LEDs
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	GPIOPinConfigure(GPIO_PK5_EN0LED2);
	GPIOPinConfigure(GPIO_PK6_EN0LED1);
	GPIOPinTypeEthernetLED(GPIO_PORTK_BASE, GPIO_PIN_5 | GPIO_PIN_6);

	// Set INT Priority
	IntPrioritySet(INT_EMAC0, (1 << 5)); // high priority?

	// Get MAC Address, from flash!
	unsigned char pucMACAddress[6];
	pucMACAddress[0] = 0x10;
	pucMACAddress[1] = 0x20;
	pucMACAddress[2] = 0x30;
	pucMACAddress[3] = 0x00;
	pucMACAddress[4] = 0x00;
	pucMACAddress[5] = 0x65;


	// lwIP Ethernet
	lwIPInit(g_ui32SysClock, pucMACAddress, inet_addr("101.1.0.10"), inet_addr("0.255.255.255"), inet_addr("1.1.0.10"), IPADDR_USE_STATIC); 	// Set IP
	//lwIPInit(g_ui32SysClock, pucMACAddress, 0, 0, 0, IPADDR_USE_DHCP); 	// Set IP

	// Fix LEDs
	HWREG(EMAC0_BASE + EMAC_O_CC) |= EMAC_CC_POL; // Set Ethernet LED polarity to be active low
	EMACPHYExtendedWrite(EMAC0_BASE, 0, EPHY_LEDCFG,(EPHY_LEDCFG_LED1_LINK | EPHY_LEDCFG_LED2_RXTX) );
	EMACPHYExtendedWrite(EMAC0_BASE, 0, EPHY_LEDCR, EPHY_LEDCR_BLINKRATE_20HZ);

	// Setup the device locator service. (can be found by "finder.exe")
	LocatorInit();
	LocatorMACAddrSet(pucMACAddress);
	LocatorAppTitleSet("EthBridge Board");

	// init lwIP
	m_udpPcb =  udp_new();
	udp_bind(m_udpPcb, IP_ADDR_ANY, ETHPORT);
	udp_recv(m_udpPcb, EtherUDPRecv, this );

	DestinationPort = 0;

	ReceivedFrames = 0;
	SentFrames = 0;
}

void EtherDriver::DataReceived(pbuf *p, ip_addr *addr, u16_t port)
{
	// process incoming data, send response back to receiver
	bool ok = IsPacketValid(p); // check magic codes

	if( ok )
	{
		// process packet
		unsigned char* data = (unsigned char*)p->payload;
		int dataSize = p->len-3;
		ReceivedFrames++;

		switch( data[2] )
		{
			case 0x10: // PING
			{
				u16_t destPort;
				memcpy(&destPort, &data[3], 2); // extract destination port
				// copy target address
				DestinationPort = destPort;
				DestinationAddr = *addr;
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

	SendPacket(type, txBuff, size, &DestinationAddr, DestinationPort);

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

	SentFrames++;
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
