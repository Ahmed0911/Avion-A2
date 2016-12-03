/*
 * BaroDrv.cpp
 *
 *  Created on: Jun 30, 2015
 *      Author: Sara
 */

#include "BaroDrv.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/ssi.h"

extern uint32_t g_ui32SysClock;

// SPI
// ---------
// 20MHz max
// Freescale SPI
// SPO = 0,SPH = 0 -> FRF_MOTO_MODE_0
// SSI2
// PINS:
// PD3 - CLK
// PD1 - MOSI
// PD0 - MISO
// PQ0 - CSB
void BaroDrv::Init()
{
	// Enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
	// SPI Pins
	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
	// Chip Select Pins - CSB
	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // default to 1
	GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_0);
	// Configure
	SSIConfigSetExpClk(SSI2_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 10000000, 8); // MODE0 and MODE3 are OK (MODE3 used on MPU9250!)
	// Enable
	SSIEnable(SSI2_BASE);
	// Clear FIFO
	uint32_t data;
	while(SSIDataGetNonBlocking(SSI2_BASE, &data)) { };

	// Reset
	Reset();

	// Read Constants
	C1 = ReadProm(0xA2);
	C2 = ReadProm(0xA4);
	C3 = ReadProm(0xA6);
	C4 = ReadProm(0xA8);
	C5 = ReadProm(0xAA);
	C6 = ReadProm(0xAC);

	// Warmup
	Update();
	SysCtlDelay(g_ui32SysClock/3/100); // 10ms delay
	Update();
	SysCtlDelay(g_ui32SysClock/3/100); // 10ms delay
	Update();
	SysCtlDelay(g_ui32SysClock/3/100); // 10ms delay
}

void BaroDrv::Reset()
{
	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, 0 ); // SELECT

	SSIDataPut(SSI2_BASE, 0x1E);
	while(SSIBusy(SSI2_BASE));
	SysCtlDelay(g_ui32SysClock/3/100); // 10ms delay-reset

	uint32_t data;
	SSIDataGet(SSI2_BASE, &data);

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // DESELECT
}

unsigned short BaroDrv::ReadProm(unsigned char address)
{
	unsigned short promConst = 0;

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, 0 ); // SELECT

	// Send data/receive
	SSIDataPut(SSI2_BASE, address);
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	while(SSIBusy(SSI2_BASE));

	uint32_t data;
	SSIDataGet(SSI2_BASE, &data); // dummy
	data &= 0x00FF;
	SSIDataGet(SSI2_BASE, &data); // MSB
	data &= 0x00FF;
	promConst = data << 8;
	SSIDataGet(SSI2_BASE, &data); // LSB
	data &= 0x00FF;
	promConst += data;

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // DESELECT

	return promConst;
}

void BaroDrv::StartConversion(unsigned char D12Command)
{
	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, 0 ); // SELECT

	// Send data/receive
	SSIDataPut(SSI2_BASE, D12Command);
	while(SSIBusy(SSI2_BASE));

	uint32_t data;
	SSIDataGet(SSI2_BASE, &data); // dummy

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // DESELECT
}

unsigned int BaroDrv::ReadADC()
{
	unsigned int adcData = 0;

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, 0 ); // SELECT

	// Send data/receive
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	while(SSIBusy(SSI2_BASE));

	uint32_t data;
	SSIDataGet(SSI2_BASE, &data); // dummy
	SSIDataGet(SSI2_BASE, &data); // HSB
	data &= 0x00FF;
	adcData = (data << 16);
	SSIDataGet(SSI2_BASE, &data); // MSB
	data &= 0x00FF;
	adcData += (data << 8);
	SSIDataGet(SSI2_BASE, &data); // LSB
	data &= 0x00FF;
	adcData += data;

	GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_0, GPIO_PIN_0 ); // DESELECT

	return adcData;
}

// Call every 10ms, NOT less!!!
void BaroDrv::Update()
{
	IntMasterDisable();
	unsigned int data = ReadADC();
	if( m_ReadTemperaturePhase )
	{
		D1 = data;
		// start D2 Conversion
		StartConversion(0x58); // 0x40 - D2-256, 0x48 - D2-4096 (10ms conversion)
		m_ReadTemperaturePhase = false;
	}
	else
	{
		D2 = data;
		// Start D1 Conversion
		StartConversion(0x48); // 0x40 - D1-256, 0x48 - D1-4096
		m_ReadTemperaturePhase = true;
	}
	IntMasterEnable();

	// Calculate Pressure
	int32_t dT = D2 - C5*256;
	int32_t TEMP = 2000 + (int64_t)dT*C6/8388608;
	int64_t OFF = (int64_t)C2*65536 + ((int64_t)C4*dT)/128;
	int64_t SENS = (int64_t)C1*32768 + ((int64_t)C3*dT)/256;
	int64_t P = (D1 * SENS/2097152 - OFF)/32768;

	TemperatureC = TEMP/100.0f;
	PressurePa = P;
}
