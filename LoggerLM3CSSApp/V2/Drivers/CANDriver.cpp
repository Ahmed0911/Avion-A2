
#include "CANDriver.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include "inc/hw_can.h"
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/can.h>
#include <string.h>
#include "EtherDriver.h"
#include "../Datafile.h"

extern SDataFile datafile;
extern EtherDriver etherDrv;

void CANDriver::Init()
{
	// set pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinConfigure(GPIO_PF0_CAN1RX);
	GPIOPinConfigure(GPIO_PF1_CAN1TX);
	GPIOPinTypeCAN(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// enable CAN1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
	CANInit(CAN1_BASE);
	CANBitRateSet(CAN1_BASE, SysCtlClockGet(), 1000000);

	// enable interrupts?
	CANIntEnable(CAN1_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
	IntEnable(INT_CAN1);

	// Enable the CAN for operation.
	CANEnable(CAN1_BASE);

	// Add RX FIFO
	tCANMsgObject sCANMessage;
	sCANMessage.ulMsgID = 0;   // receive ALL
	sCANMessage.ulMsgIDMask = 0;
	sCANMessage.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER | MSG_OBJ_FIFO; // generate RX INT
	sCANMessage.ulMsgLen = 8;       // allow up to 8 bytes
	CANMessageSet(CAN1_BASE, 1, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 2, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 3, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 4, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 5, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 6, &sCANMessage, MSG_OBJ_TYPE_RX);
	CANMessageSet(CAN1_BASE, 7, &sCANMessage, MSG_OBJ_TYPE_RX);
	sCANMessage.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER; // last message in FIFO
	CANMessageSet(CAN1_BASE, 8, &sCANMessage, MSG_OBJ_TYPE_RX);
}

bool CANDriver::SendMessage(int msgID, unsigned char data[8], int len)
{
	// find first free object
	unsigned long ulTxMask = CANStatusGet(CAN1_BASE, CAN_STS_TXREQUEST);
	for(int i=9; i!= 32; i++)
	{
		unsigned long bit = 1 << (i-1);
		if( (ulTxMask & bit) == 0)
		{
			// free, use for next transfer
			SendMessage(i, msgID, data, len);
			datafile.TXSent++;
			return true;
		}
	}

	datafile.TXOverrun++;
	return false; // failed!
}

void CANDriver::SendMessage(int objectIdx, int msgID, unsigned char data[8], int len)
{
	tCANMsgObject sCANMessage;

	sCANMessage.ulMsgID = msgID;                        // CAN message ID
	sCANMessage.ulMsgIDMask = 0;                    // no mask needed for TX
	sCANMessage.ulFlags = 0;// | MSG_OBJ_TX_INT_ENABLE;    // enable interrupt on TX
	sCANMessage.ulMsgLen = len;
	sCANMessage.pucMsgData = data;             // ptr to message content

	CANMessageSet(CAN1_BASE, objectIdx, &sCANMessage, MSG_OBJ_TYPE_TX);
}
/*
void CANDriver::SetRXMessage(int objectIdx, int msgID, int len)
{
	tCANMsgObject sCANMessage;

	sCANMessage.ulMsgID = msgID;                        // CAN message ID
	sCANMessage.ulMsgIDMask = 0;
	sCANMessage.ulFlags = MSG_OBJ_EXTENDED_ID;
	sCANMessage.ulMsgLen = len;       // size of message is 4

	CANMessageSet(CAN1_BASE, objectIdx, &sCANMessage, MSG_OBJ_TYPE_RX);
}
*/
/*
bool CANDriver::ReadMessage(int objectIdx, unsigned char data[8])
{
	tCANMsgObject sCANMessage;
	sCANMessage.pucMsgData = data;

	CANMessageGet(CAN1_BASE, objectIdx, &sCANMessage, 1 );
	bool newData = sCANMessage.ulFlags & MSG_OBJ_NEW_DATA;

	return newData;
}
*/

