#include "LEDDriver.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>

void LEDDriver::Init()
{
	// Enable the GPIO port that is used for the on-board LEDs.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    // Enable the GPIO pin for the LED (PD0).  Set the direction as output, and
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_7);
    
    // reset all
    Reset(LEDGREEN);
}

void LEDDriver::Set(ELED led)
{
	switch( led )
	{
		case LEDGREEN:
			GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_7, 0x00);
			break;
	}	
}

void LEDDriver::Reset(ELED led)
{
	switch( led )
	{
		case LEDGREEN:
			GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_7, GPIO_PIN_7);
			break;
	}		
}
