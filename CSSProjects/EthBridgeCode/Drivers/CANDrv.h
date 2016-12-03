/*
 * CANDrv.h
 *
 *  Created on: Aug 11, 2016
 *      Author: Ivan
 */

#ifndef DRIVERS_CANDRV_H_
#define DRIVERS_CANDRV_H_

class CANDrv
{
public:
	void Init();
	void Update();

	bool SendMessage(int id, unsigned char* data, int len);
	bool GetMessage(int& id, unsigned char* data, int& len);

private:
	// debug
	int dbgStatusWord;
	int dbgTXSent;
	int dbgTXFails;
	int dbgTXLastObjID;
	int dbgTXMaxObjID;
	int dbgRXRecv;
	int dbgRXLastObjID;
	int dbgRXMaxObjID;

};

#endif /* DRIVERS_CANDRV_H_ */
