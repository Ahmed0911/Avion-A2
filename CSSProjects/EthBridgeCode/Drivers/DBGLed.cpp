/*
 * DBGLed.cpp
 *
 *  Created on: Jun 26, 2015
 *      Author: Sara
 */

#include "DBGLed.h"
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

void DBGLed::Init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_2);
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
	Reset();
}

void DBGLed::Set(bool set)
{
	if(set) Set();
	else Reset();
}

void DBGLed::Set()
{
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_PIN_2 );
	m_LastState = true;
}

void DBGLed::Reset()
{
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0 );
	m_LastState = false;
}

void DBGLed::Toggle()
{
	if( m_LastState ) Reset();
	else Set();
}
