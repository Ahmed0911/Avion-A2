/*
 * SerialDriver.h
 *
 *  Created on: Oct 8, 2014
 *      Author: Sara
 */

#ifndef SERIALDRIVER_H_
#define SERIALDRIVER_H_

#include "../Fifo.h"

class SerialDriver
{
public:
	void Init(unsigned int portbase, unsigned int baud);
	int Write(BYTE* buffer, int size);
	int Read(BYTE* buffer, int size);

	void IntHandler(void);

private:
	Fifo m_TXFifo;
	Fifo m_RXFifo;

	int m_SentBytes;
	int m_ReceivedBytes;

	void WriteProcess();

	unsigned int m_BaseAddr;
};

#endif /* SERIALDRIVER_H_ */
