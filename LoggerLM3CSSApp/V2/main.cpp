#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>
#include <driverlib/systick.h>
#include <driverlib/can.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "Drivers/LEDDriver.h"
#include "Drivers/EtherDriver.h"
#include "Drivers/CANDriver.h"
#include "Drivers/SDCardDriver.h"
#include "Drivers/MPU6000Drv.h"
#include "Datafile.h"
#include "Timer.h"

#define SYSCLKHZ 100

SDataFile datafile;
LEDDriver ledDrv;
Timer timer;
//EtherDriver etherDrv;
SDCardDriver sdCard;
CANDriver canDriver;
MPU6000Drv mpuDrv;
bool goFlush = false;

void ProcessCommand(int cmd, unsigned char* data, int dataSize);

SCommEthData ethDataStruct;

int main(void)
{
	// Set the clocking to run at 80 MHz from the PLL.
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // 80MHZ (400MHz/2.5 = 80MHz)

	// zero struct
	memset(&ethDataStruct, 0, sizeof(ethDataStruct));

	// Initialize Drivers
	timer.Init();
	ledDrv.Init();
	sdCard.Init();
	//etherDrv.Init();
	canDriver.Init();
	mpuDrv.Init();

	// Configure SysTick
	SysTickPeriodSet(SysCtlClockGet()/SYSCLKHZ); // 100 Hz Timer
	IntPrioritySet(FAULT_SYSTICK, (3 << 5)); // priority = 3! (0x00000000-lowest, 0x11100000-highest)
	SysTickIntEnable();
	SysTickEnable();

	// enable INTs
	IntMasterEnable();

	while(1)
	{
		timer.Start();

		// write to card + flush
		sdCard.WriteChunks();

		if( goFlush )
		{
			// FLUSH
			sdCard.Flush();
			goFlush = false; // reset flag
		}

		// Loop time
		datafile.LoopTimeMS = timer.GetMS();
		if( datafile.LoopTimeMS > datafile.LoopTimeMSMAX ) datafile.LoopTimeMSMAX = datafile.LoopTimeMS;
	}
}

// Process commands received from Ethernet
void ProcessCommand(int cmd, unsigned char* data, int dataSize)
{
	switch( cmd )
	{
		case 0x20:
		{
			// data received
			if( dataSize == sizeof(SCommEthData)) // check length
			{
				// Chunk data
				sdCard.ChunkData(data, dataSize);
			}
		}
	}
}

extern "C" void SysTickIntHandler(void)
{
	datafile.MissionTime += (timer.GetMS()/1000); // sec
    datafile.Ticks++; // tick counter

    // process ethernet (RX)
    //etherDrv.Process(10);

    // Process SDCARD
    disk_timerproc();

	if( (datafile.Ticks%100) == 0)
	{
		// send ping to A2 Controller every second
		// MAX: 10, 20, 30, 00, 00, 7A
		// IP: 10.0.1.121, 255.255.255.0
		// PORT: 12000
		//SPingLoggerData pingData;
		//pingData.DestinationPort = ETHPORT;
		//pingData.SDCardActive = datafile.SDCardActive;
		//pingData.SDCardBytesWritten = datafile.SDCardBytesWritten;
		//pingData.SDCardFails = datafile.SDCardFails;
		//pingData.FailedQueues = datafile.FailedQueues;
		//unsigned int addr = inet_addr("10.0.1.121");
		//etherDrv.SendPacket(0x11, (char*)&pingData, sizeof(pingData), (ip_addr*)&addr, 12000 );

		BYTE msg[8];
		unsigned int cardActive = datafile.SDCardActive;
		memcpy(&msg[0], &cardActive, 4);
		memcpy(&msg[4], &datafile.SDCardBytesWritten, 4);
		canDriver.SendMessage(0x200, msg, 8 );

		memcpy(&msg[0], &datafile.SDCardFails, 4);
		memcpy(&msg[4], &datafile.FailedQueues, 4);
		canDriver.SendMessage(0x201, msg, 8 );

	}

    // read MPU-6000 (UNUSED!!!)
    //mpuDrv.UpdateData();
    //mpuDrv.GetGyro(datafile.MPUGyroX, datafile.MPUGyroY, datafile.MPUGyroZ );
    //mpuDrv.GetAcc(datafile.MPUAccX, datafile.MPUAccY, datafile.MPUAccZ );
    //mpuDrv.GetTemperature(datafile.MPUTemperature);


    // periodic flush
    if( (datafile.Ticks%100) == 0) goFlush = true;

    // Blink LEDs
    if( datafile.SDCardActive && datafile.SDCardFails == 0)
    {
		if( (datafile.Ticks%100) < 50) ledDrv.Set(LEDDriver::LEDGREEN);
		else ledDrv.Reset(LEDDriver::LEDGREEN);
    }
    else
    {
    	// CARD FAIL
    	if( (datafile.Ticks%20) < 10) ledDrv.Set(LEDDriver::LEDGREEN);
    	else ledDrv.Reset(LEDDriver::LEDGREEN);
    }
}

int CANMsgReceivedCount = 0;
int CANMsgReceivedMaxIndex = 0;

extern "C" void CANDriverINTHandler(void)
{
	// Read the CAN interrupt status to find the cause of the interrupt
	unsigned long ulStatus = CANIntStatus(CAN1_BASE, CAN_INT_STS_CAUSE);

	// If the cause is a controller status interrupt, then get the status
	if(ulStatus == CAN_INT_INTID_STATUS)
	{
		// Read the controller status.  This will return a field of status
		// error bits that can indicate various errors.
		ulStatus = CANStatusGet(CAN1_BASE, CAN_STS_CONTROL);
		// TODO: handle error!
	}
	else if(ulStatus >= 1 && ulStatus <= 8) // Check if the cause is from RX FIFO objects (1...8)
	{
		// check all objects
		for(int i=1; i!=8; i++)
		{
			unsigned char data[8];
			tCANMsgObject sCANMessage;
			sCANMessage.pucMsgData = data;
			CANMessageGet(CAN1_BASE, i, &sCANMessage, 1);
			if( sCANMessage.ulFlags & MSG_OBJ_NEW_DATA )
			{
				CANMsgReceivedCount++;
				if( i > CANMsgReceivedMaxIndex) CANMsgReceivedMaxIndex = i;
				int ID = sCANMessage.ulMsgID;

				// Copy to struct
				// pack and go
				BYTE* ptr = (BYTE*)&ethDataStruct;
				if( ID >= 0x100 && ID <=0x11D)
				{
					int index = ID-0x100;
					memcpy(&ptr[index*8], data, 8 );

					if( ID == 0x11D)
					{
						// initiate store
						sdCard.ChunkData((BYTE*)&ethDataStruct, sizeof(ethDataStruct));
					}
				}
			}
		}
	}
	else
	{
		// Spurious interrupt handling can go here.
	}
}
