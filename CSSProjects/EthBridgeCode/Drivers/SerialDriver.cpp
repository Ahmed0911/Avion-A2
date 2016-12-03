/*
 * SerialDriver.cpp
 *
 *  Created on: Oct 8, 2014
 *      Author: Sara
 */

#include "SerialDriver.h"
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"

extern uint32_t g_ui32SysClock;

void SerialDriver::IntHandler(void)
{
	uint32_t ui32Ints;

	// Get and clear the current interrupt source(s)
	ui32Ints = UARTIntStatus(m_BaseAddr, true);
	UARTIntClear(m_BaseAddr, ui32Ints);

	// Are we being interrupted because the TX FIFO has space available?
	if(ui32Ints & UART_INT_TX)
	{
		WriteProcess(); // send data to TX FIFO

		if( m_TXFifo.IsEmpty() ) UARTIntDisable(m_BaseAddr, UART_INT_TX); // no more data buffer, disable TX INT
	}

	// Are we being interrupted due to a received character?
	if(ui32Ints & (UART_INT_RX | UART_INT_RT))
	{
	   // Get all the available characters from the UART.
	   while(UARTCharsAvail(m_BaseAddr))
	   {
		   int32_t i32Char = UARTCharGetNonBlocking(m_BaseAddr);
		   BYTE ch =  (BYTE)(i32Char & 0xFF);

		   m_RXFifo.Push(ch);
		   m_ReceivedBytes++;
	   }
	}
}

void SerialDriver::Init(unsigned int portbase, unsigned int baud)
{
	// setup
	m_BaseAddr = portbase;

	m_SentBytes = 0;
	m_ReceivedBytes = 0;

	if( portbase == UART0_BASE) // HACK!
	{
		// port A
		// pins PA0 - U0RX
		// pins PA1 - U0TX
		//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
		//SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
		//GPIOPinConfigure(GPIO_PA0_U0RX);
		//GPIOPinConfigure(GPIO_PA1_U0TX);
		//GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

		//IntEnable(INT_UART0);
	}
	else if( portbase == UART2_BASE) // HACK!
	{
		// port A
		// pins PD4 - U2RX
		// pins PD5 - U2TX
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
		GPIOPinConfigure(GPIO_PD4_U2RX);
		GPIOPinConfigure(GPIO_PD5_U2TX);
		GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

		IntEnable(INT_UART2);
	}
	else if( portbase == UART3_BASE) // HACK!
	{
		// port A
		// pins PA4 - U3RX
		// pins PA5 - U3TX
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
		GPIOPinConfigure(GPIO_PA4_U3RX);
		GPIOPinConfigure(GPIO_PA5_U3TX);
		GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5);

		IntEnable(INT_UART3);
	}
	else if( portbase == UART5_BASE) // HACK!
	{
		// port C
		// pins PC6 - U5RX
		// pins PC7 - U5TX
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
		GPIOPinConfigure(GPIO_PC6_U5RX);
		GPIOPinConfigure(GPIO_PC7_U5TX);
		GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);

		IntEnable(INT_UART5);
	}

	// HACK for PORT3 + SBUS
	if( portbase == UART3_BASE )	UARTConfigSetExpClk(m_BaseAddr, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO | UART_CONFIG_PAR_EVEN));
	else UARTConfigSetExpClk(m_BaseAddr, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	UARTTxIntModeSet(m_BaseAddr, UART_TXINT_MODE_FIFO);
	UARTFIFOLevelSet(m_BaseAddr, UART_FIFO_TX2_8, UART_FIFO_RX4_8); // set RX/TX FIFO levels

	UARTIntEnable(m_BaseAddr, UART_INT_RX | UART_INT_RT); // RT - receive timeout: RX FIFO not empty but no new data for 32 periods (flush)
	UARTEnable(m_BaseAddr); // FIFOs are also enabled by this function, no need for UARTFIFOEnable()

	//IntMasterEnable(); // Enable processor interrupts.
}

int SerialDriver::Read(BYTE* buffer, int size)
{
	int read = 0;
	IntMasterDisable();

	// get data from RX FIFO
	for(int i=0; i!=size; i++)
	{
		BYTE ch;
		if( m_RXFifo.Pop(ch) ) // is data available?
		{
			buffer[i] = ch;
			read++;
		}
		else break; // no more data
	}
	IntMasterEnable();

	return read;
}

int SerialDriver::Write(BYTE* buffer, int size)
{
	int written = 0;
	IntMasterDisable();

	// push data to TX FIFO
	for(int i=0; i != size; i++)
	{
		if( m_TXFifo.Push(buffer[i]) ) // fifo not full?
		{
			written++; // stored to FIFO
		}
	}

	WriteProcess(); // fill TX FIFO
	if( !m_TXFifo.IsEmpty() ) UARTIntEnable(m_BaseAddr, UART_INT_TX); // more data to TX FIFO to send, enable TX interrupt

	IntMasterEnable();

	return written;
}

void SerialDriver::WriteProcess()
{
	while( UARTSpaceAvail(m_BaseAddr) && !m_TXFifo.IsEmpty() )
	{
		BYTE ch;
		m_TXFifo.Pop(ch);
		UARTCharPutNonBlocking(m_BaseAddr, ch);
		m_SentBytes++;
	};
}
