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
#include "Drivers/MPU6000Drv.h"
#include "Datafile.h"
#include "Timer.h"

#define SYSCLKHZ 100

SDataFile datafile;
LEDDriver ledDrv;
Timer timer;
EtherDriver etherDrv;
CANDriver canDriver;
MPU6000Drv mpuDrv;

int main(void)
{
	// Set the clocking to run at 80 MHz from the PLL.
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // 80MHZ (400MHz/2.5 = 80MHz)

	// Initialize Drivers
	timer.Init();
	ledDrv.Init();
	etherDrv.Init();
	canDriver.Init();
	mpuDrv.Init();

	// Configure SysTick
	SysTickPeriodSet(SysCtlClockGet()/SYSCLKHZ); // 100 Hz Timer
	IntPrioritySet(FAULT_SYSTICK, (3 << 5)); // priroty = 3! (0x00000000-lowest, 0x11100000-highest)
	SysTickIntEnable();
	SysTickEnable();

	// enable INTs
	IntMasterEnable();

	while(1)
	{
		// add me
	}
}

extern "C" void SysTickIntHandler(void)
{
	datafile.MissionTime += (timer.GetMS()/1000); // sec
	timer.Start();
    datafile.Ticks++; // tick counter

    // process ethernet (RX)
    etherDrv.Process(10);

    // read MPU-6000
    mpuDrv.UpdateData();
    mpuDrv.GetGyro(datafile.MPUGyroX, datafile.MPUGyroY, datafile.MPUGyroZ );
    mpuDrv.GetAcc(datafile.MPUAccX, datafile.MPUAccY, datafile.MPUAccZ );
    mpuDrv.GetTemperature(datafile.MPUTemperature);

    // Blink LEDs
    //if( (datafile.Ticks%100) < 50) ledDrv.Set(LEDDriver::LEDGREEN);
    //else ledDrv.Reset(LEDDriver::LEDGREEN);

    // looptime
    datafile.LoopTimeMS = timer.GetMS();
    if( datafile.LoopTimeMS > datafile.LoopTimeMSMAX ) datafile.LoopTimeMSMAX = datafile.LoopTimeMS;
}
