#include "stdafx.h"
#include "Fifo.h"
#include <string.h>

Fifo::Fifo()
{
	memset(m_Fifo, 0x00, FIFOSIZE); // clear fifo (debug)
	m_IdxPush = 0;
	m_IdxPop = 0;
}


Fifo::~Fifo()
{

}

bool Fifo::Push(BYTE ch)
{
	if (IsFull() ) return false; // fifo full!
	
	m_Fifo[m_IdxPush] = ch;
	m_IdxPush = FixIndex(++m_IdxPush);

	return true;
}

bool Fifo::Pop(BYTE& ch)
{
	if (IsEmpty()) return false; // fifo empty
	
	ch = m_Fifo[m_IdxPop];
	m_IdxPop = FixIndex(++m_IdxPop);

	return true;
}

bool Fifo::IsFull()
{
	bool isFull = false;
	if (FixIndex(m_IdxPush + 1) == m_IdxPop) isFull = true;

	return isFull;
}

bool Fifo::IsEmpty()
{
	bool isEmpty = false;
	if (m_IdxPop == m_IdxPush) isEmpty = true;
	
	return isEmpty;
}

int Fifo::FixIndex(int index)
{
	if (index >= FIFOSIZE)
	{
		index -= FIFOSIZE;
	}

	return index;
}