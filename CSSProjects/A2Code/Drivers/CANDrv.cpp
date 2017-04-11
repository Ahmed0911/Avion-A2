/*
 * CANDrv.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: Ivan
 */

#include "CANDrv.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/can.h"
#include "driverlib/sysctl.h"

extern uint32_t g_ui32SysClock;

#define TXBUFFS 30

// TX Objects: CH1....CH30
// RX Objects: CH31...CH32
void CANDrv::Init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);

	GPIOPinConfigure(GPIO_PB0_CAN1RX);
	GPIOPinConfigure(GPIO_PB1_CAN1TX);
	GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	CANInit(CAN1_BASE);
	CANBitRateSet(CAN1_BASE, g_ui32SysClock, 1000000); // set CAN to 1000kbit/s
	CANEnable(CAN1_BASE);

	// init counters
	dbgStatusWord = 0;
	dbgTXSent = 0;
	dbgTXFails = 0;
	dbgTXLastObjID = 0;
	dbgTXMaxObjID = 0;
	dbgRXRecv = 0;
	dbgRXLastObjID = 0;
	dbgRXMaxObjID = 0;

	// Init RX Objects
	tCANMsgObject sCANMessage;
	sCANMessage.ui32MsgID = 0; // receive ALL!!!
	sCANMessage.ui32MsgIDMask = 0;
	sCANMessage.ui32Flags = MSG_OBJ_USE_ID_FILTER | MSG_OBJ_FIFO;
	sCANMessage.ui32MsgLen = 8;
	for(int i=TXBUFFS+1; i<=32; i++)
	{
		CANMessageSet(CAN1_BASE, i, &sCANMessage, MSG_OBJ_TYPE_RX);
	}
	sCANMessage.ui32Flags = MSG_OBJ_USE_ID_FILTER;
	CANMessageSet(CAN1_BASE, 32, &sCANMessage, MSG_OBJ_TYPE_RX);
}

void CANDrv::Update()
{
	unsigned int stsStats = CANStatusGet(CAN1_BASE, CAN_STS_CONTROL);
	dbgStatusWord = stsStats;
}

bool CANDrv::SendMessage(int id, unsigned char* data, int len)
{
	tCANMsgObject sCANMessage;
	sCANMessage.ui32MsgID = id;
	sCANMessage.ui32MsgIDMask = 0;
	sCANMessage.ui32Flags = 0;
	sCANMessage.ui32MsgLen = len;
	sCANMessage.pui8MsgData = data;

	// find free CAN object
	unsigned int txStatus = CANStatusGet(CAN1_BASE, CAN_STS_TXREQUEST);
	for(int i=1; i<=TXBUFFS; i++)
	{
		unsigned int bitX = 1 << (i-1);
		if( (txStatus & bitX) == 0)
		{
			// object ready, send
			CANMessageSet(CAN1_BASE, i, &sCANMessage, MSG_OBJ_TYPE_TX);
			dbgTXLastObjID = i;
			if( i > dbgTXMaxObjID ) dbgTXMaxObjID = i;
			dbgTXSent++;

			return true;
		}
	}

	dbgTXFails++;
	return false;
}

bool CANDrv::GetMessage(int& id, unsigned char* data, int& len)
{
	tCANMsgObject sCANMessage;

	unsigned int rxStatus = CANStatusGet(CAN1_BASE, CAN_STS_NEWDAT);
	for(int i=TXBUFFS+1; i<=32; i++)
	{
		// data available?
		unsigned int bitX = 1 << (i-1);
		if( (rxStatus & bitX) != 0)
		{
			// get data
			sCANMessage.pui8MsgData = data;
			CANMessageGet(CAN1_BASE, i, &sCANMessage, false);
			id = sCANMessage.ui32MsgID;
			len = sCANMessage.ui32MsgLen;
			dbgRXLastObjID = i;
			if( i > dbgRXMaxObjID ) dbgRXMaxObjID = i;
			dbgRXRecv++;

			return true;
		}
	}

	return false;
}
