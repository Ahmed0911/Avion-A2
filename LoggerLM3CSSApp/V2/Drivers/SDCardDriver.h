
#ifndef SDCARDDRIVER_H_
#define SDCARDDRIVER_H_

#include "../Datafile.h"

extern "C"
{
#include "ff.h"
#include "diskio.h"
}

class SDCardDriver
{
public:
	bool Init();
	bool WriteData(BYTE* data, int len);
	bool Flush(); // call from main loop every second or more

private:

	FATFS m_sFatFs;
	FIL m_sFileObject;
	bool m_FileValid;
};

#endif /* SDCARDDRIVER_H_ */
