#pragma once
typedef unsigned char BYTE;

#define FIFOSIZE 1024

class Fifo
{
public:
	Fifo();
	~Fifo();

	bool Push(BYTE ch);
	bool Pop(BYTE& ch);

	bool IsFull();
	bool IsEmpty();
	bool HasSpace(int requestedSpace);
	int Count();

private:
	BYTE m_Fifo[FIFOSIZE];
	int m_IdxPush; // index for next push
	int m_IdxPop; // index for next pop (if equal->fifo is empty)

	int FixIndex(int index);
};

