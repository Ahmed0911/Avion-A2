/*
 * MPU9250Drv.cpp
 *
 *  Created on: Jul 1, 2015
 *      Author: Sara
 */

#include "MPU9250Drv.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/ssi.h"

extern uint32_t g_ui32SysClock;

#define SELECT()  GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_2, 0 );
#define DESELECT()  GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_2, GPIO_PIN_2 );

// SPI
// ---------
// 1MHz, 20MHz max (read only)
// Freescale SPI
// SPO = 1,SPH = 1 -> FRF_MOTO_MODE_3
// SSI2
// PINS:
// PD3 - CLK
// PD1 - MOSI
// PD0 - MISO
// PK2 - CS_G2
// PK3 - INT_G2
bool MPU9250Drv::Init()
{
	m_MPU9150INTCounter = 0;

	// Enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	// SPI Pins
	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
	// Chip Select Pins - CSB
	GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_2, GPIO_PIN_2 ); // default to 1
	GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_2);
	// INT Pin
	GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_3);
	// Set AS INT Source!!!
	GPIOIntEnable(GPIO_PORTK_BASE, GPIO_PIN_3);
	GPIOIntTypeSet(GPIO_PORTK_BASE, GPIO_PIN_3, GPIO_FALLING_EDGE);
	IntEnable(INT_GPIOK);

	// Configure
	SSIConfigSetExpClk(SSI2_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 1000000, 8);
	// Enable
	SSIEnable(SSI2_BASE);

	// Clear fifo
	ClearFIFO();

	// check ID
	unsigned char buf[10];
	ReadBytes(0x75, buf, 1 ); //WHOAMI (must return 0x71)
	if( buf[0] != 0x71 ) return false;

	// Init
	// 1. Reset
	buf[0] = MPU9150_PWR_MGMT_1_DEVICE_RESET;
	WriteBytes(MPU9150_O_PWR_MGMT_1, buf, 1);
	SysCtlDelay(g_ui32SysClock/3/20); // 50ms delay -> RESET WAIT

	// 2. Read  MPU9150_O_PWR_MGMT_1 check reset completed! (MPU9150_PWR_MGMT_1_SLEEP is SET???) -> Doesn't work!
	//do
	//{
	//	buf[0] = 0;
	//	ReadBytes(MPU9150_O_PWR_MGMT_1, buf, 1);
	//} while(buf[0] != MPU9150_PWR_MGMT_1_SLEEP );

	// 3. Exit Sleep Mode
	buf[0] = MPU9150_PWR_MGMT_1_CLKSEL_XG;
	WriteBytes(MPU9150_O_PWR_MGMT_1, buf, 1);

	// 4. I2C master mode enable
	buf[0] = MPU9150_USER_CTRL_I2C_MST_EN;
	WriteBytes(MPU9150_O_USER_CTRL, buf, 1);

	// 5. Set Sample rate, Set sample rate to 500 hertz.  1000 hz / (1 + 1) = 1000Hz / 2 = 500Hz
	buf[0] = 1; // 500 Hz
	WriteBytes(MPU9150_O_SMPLRT_DIV, buf, 1);

	// 6. Write the I2C Master delay control so we only sample the AK every 5th time that we sample accel/gyro.  Delay Count itself handled in next state.
	buf[0] = (MPU9150_I2C_MST_DELAY_CTRL_I2C_SLV0_DLY_EN | MPU9150_I2C_MST_DELAY_CTRL_I2C_SLV4_DLY_EN);
	WriteBytes(MPU9150_O_I2C_MST_DELAY_CTRL, buf, 1);

	// 7. Write the configuration for I2C master control clock 400khz and wait for external sensor before asserting data ready, Configure I2C Slave 0 for read of AK8975 (I2C Address 0x0C), Start at AK8975 register status 1, Read 8 bytes and enable this slave transaction
	buf[0] = (MPU9150_I2C_MST_CTRL_I2C_MST_CLK_400 | MPU9150_I2C_MST_CTRL_WAIT_FOR_ES);
	buf[1] = (MPU9150_I2C_SLV0_ADDR_RW | 0x0C);
	buf[2] = AK8975_O_ST1;
	buf[3] = (MPU9150_I2C_SLV0_CTRL_EN | 0x08);
	WriteBytes(MPU9150_O_I2C_MST_CTRL, buf, 4);

	// 9. Write the configuration for I2C Slave 4 transaction to AK8975 0x0c is the AK8975 address on i2c bus. we want to write the control register with the value for a starting a single measurement.
	// 		Enable the SLV4 transaction and set the master delay to 0x04 + 1.  This means the slave transactions with delay enabled will run every fifth accel/gyro sample.
	buf[0] = 0x0C;
	buf[1] = AK8975_O_CNTL;
	buf[2] = AK8975_CNTL_MODE_SINGLE;
	buf[3] = (MPU9150_I2C_SLV4_CTRL_EN | 0x04); // 1/5 -> 100Hz on 500Hz ODR
	WriteBytes(MPU9150_O_I2C_SLV4_ADDR, buf, 4);

	// 10. Write application specific sensor configuration such as filter settings and sensor range settings.
	//buf[0] = MPU9150_CONFIG_DLPF_CFG_460_250; // Gyro LPF -> Output at 8KHz!
	buf[0] = MPU9150_CONFIG_DLPF_CFG_20_20; // Gyro LPF
	buf[1] = MPU9150_GYRO_CONFIG_FS_SEL_500;
	buf[2] = MPU9150_ACCEL_CONFIG_AFS_SEL_2G;
	buf[3] = MPU9150_CONFIG_DLPF_CFG_5_5; // Accel LPF
	WriteBytes(MPU9150_O_CONFIG, buf, 4);

	// 11. Configure the data ready interrupt pin output of the MPU9150.
	buf[0] = (MPU9150_INT_PIN_CFG_INT_LEVEL | MPU9150_INT_PIN_CFG_INT_RD_CLEAR | MPU9150_INT_PIN_CFG_LATCH_INT_EN);
	buf[1] = MPU9150_INT_ENABLE_DATA_RDY_EN;
	WriteBytes(MPU9150_O_INT_PIN_CFG, buf, 2);

	// 12. Switch to 10MHz SPI
	SSIDisable(SSI2_BASE);
	SSIConfigSetExpClk(SSI2_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 10000000, 8);
	SSIEnable(SSI2_BASE);

	return true;
}

void MPU9250Drv::Update()
{
	// make local copy
	IntMasterDisable();
	unsigned char data[30];
	memcpy(data, Data, 30);
	IntMasterEnable();

	unsigned int value = 0;
	value = (data[0] << 8) + data[1];
	Accel[0] = (short)value * 0.0005985482f; // [m/s^2]
	value = (data[2] << 8) + data[3];
	Accel[1] = (short)value * 0.0005985482f; // [m/s^2]
	value = (data[4] << 8) + data[5];
	Accel[2] = (short)value * 0.0005985482f; // [m/s^2]

	value = (data[8] << 8) + data[9];
	Gyro[0] = (short)value * 0.0152671755724f; // [°/s]
	value = (data[10] << 8) + data[11];
	Gyro[1] = (short)value * 0.0152671755724f; // [°/s]
	value = (data[12] << 8) + data[13];
	Gyro[2] = (short)value * 0.0152671755724f; // [°/s]

	value = (data[6] << 8) + data[7];
	Temperature = (short)value * 0.00299517f + 21; // [°C]

	value = (data[16] << 8) + data[15];
	Mag[0] = (short)value * 0.6f; // [uT]
	value = (data[18] << 8) + data[17];
	Mag[1] = (short)value * 0.6f; // [uT]
	value = (data[20] << 8) + data[19];
	Mag[2] = (short)value * 0.6f; // [uT]
}

void MPU9250Drv::MotionINT(void)
{
	unsigned int ulStatus;
	ulStatus = GPIOIntStatus(GPIO_PORTK_BASE, true);

	// Clear all the pin interrupts that are set
	GPIOIntClear(GPIO_PORTK_BASE, ulStatus);

	if(ulStatus & GPIO_PIN_3)
	{
		// MPU9150 Data is ready for retrieval and processing.
		// Read the data registers from the MPU9150. (ukupno citaj 22 bajta, pazi na HIGH/LOW od Mag data (invertirano))
		// (ACCEL_XOUT_H(0x3B) -> GYRO_ZOUT_L(0x48) = 14 bytes
		// Grab Ext Sens Data as well for another 8 bytes.  ST1 + Mag Data + ST2
		ReadBytes(MPU9150_O_ACCEL_XOUT_H, Data, 22);
		m_MPU9150INTCounter++;
	}
}

void MPU9250Drv::ClearFIFO()
{
	// Clear FIFO
	uint32_t data;
	while(SSIDataGetNonBlocking(SSI2_BASE, &data));
}

// FIFO is 8 words wide!
void MPU9250Drv::ReadBytes(unsigned char address, unsigned char* buffer, int count)
{
	uint32_t data;
	SELECT();

	SSIDataPut(SSI2_BASE, address + 0x80); // 0x80 - READ FLAG
	while(SSIBusy(SSI2_BASE));
	SSIDataGet(SSI2_BASE, &data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI2_BASE, 0x00); // READ
		while(SSIBusy(SSI2_BASE));
		SSIDataGet(SSI2_BASE, &data);
		buffer[i] = (data&0x00FF);
	}

	DESELECT();
}

void MPU9250Drv::WriteBytes(unsigned char address, unsigned char* buffer, int count)
{
	volatile uint32_t data;
	SELECT();

	SSIDataPut(SSI2_BASE, address);
	while(SSIBusy(SSI2_BASE));
	SSIDataGet(SSI2_BASE, (uint32_t*)&data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI2_BASE, buffer[i]); // WRITE
		while(SSIBusy(SSI2_BASE));
		SSIDataGet(SSI2_BASE, (uint32_t*)&data);
	}

	DESELECT();
}

