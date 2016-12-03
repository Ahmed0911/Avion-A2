/*
 * WpnOutputs.cpp
 *
 *  Created on: Jun 26, 2015
 *      Author: Sara
 */

#include "WpnOutputs.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/fpu.h"


void WpnOutputs::Init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_2, 0 );
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_2);
}

void WpnOutputs::Set(int out, bool set)
{
	if( set ) Set(out);
	else Reset(out);
}

void WpnOutputs::Set(int out)
{
	if( out == WPNOUT1) GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6 );
	if( out == WPNOUT2) GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2 );
}

void WpnOutputs::Reset(int out)
{
	if( out == WPNOUT1) GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0 );
	if( out == WPNOUT2) GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0 );
}
