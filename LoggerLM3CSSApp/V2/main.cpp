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
EtherDriver etherDrv;
SDCardDriver sdCard;
CANDriver canDriver;
MPU6000Drv mpuDrv;
bool goFlush = false;

void ProcessCommand(int cmd, unsigned char* data, int dataSize);

int main(void)
{
	// Set the clocking to run at 80 MHz from the PLL.
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // 80MHZ (400MHz/2.5 = 80MHz)

	// Initialize Drivers
	timer.Init();
	ledDrv.Init();
	sdCard.Init();
	etherDrv.Init();
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
				SCommEthData ethData;
				memcpy(&ethData, data, sizeof(ethData) );
				sdCard.ChunkData(ethData);
			}
		}
	}
}

extern "C" void SysTickIntHandler(void)
{
	datafile.MissionTime += (timer.GetMS()/1000); // sec
    datafile.Ticks++; // tick counter

    // process ethernet (RX)
    etherDrv.Process(10);

	if( (datafile.Ticks%100) == 0)
	{
		// send ping to A2 Controller every second
		// MAX: 10, 20, 30, 00, 00, 7A
		// IP: 10.0.1.121, 255.255.255.0
		// PORT: 12000
		unsigned short destPort = ETHPORT;
		unsigned int addr = inet_addr("10.0.1.121");
		etherDrv.SendPacket(0x10, (char*)&destPort, 2, (ip_addr*)&addr, 12000 );
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
