
#ifndef SDCARDDRIVER_H_
#define SDCARDDRIVER_H_

#include "../Datafile.h"
#include "../Fifo.h"

extern "C"
{
#include "ff.h"
#include "diskio.h"
}

class SDCardDriver
{
public:
	bool Init();
	bool ChunkData(BYTE* data, int size); // call from interrupt
	bool WriteChunks(); // call from main loop

	bool Flush(); // call from main loop every second or more

private:
	bool WriteData(BYTE* data, int len);
	FATFS m_sFatFs;
	FIL m_sFileObject;
	bool m_FileValid;

	// data to queue
	Fifo m_Fifo;
};

#endif /* SDCARDDRIVER_H_ */
