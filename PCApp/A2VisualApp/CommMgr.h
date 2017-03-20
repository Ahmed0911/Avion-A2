#pragma once
#include <queue>
#include "SerialComm.h"
typedef void(*ProcessMessageCallbackType)(BYTE, BYTE*, int);

// MSG QUEUE
struct SMsg
{
	BYTE Type;
	BYTE Data[255];
	BYTE Length;
};

class CCommMgr
{
public:
	// Functions
	static CCommMgr* getInstance();
	CCommMgr();
	void Open(TCHAR* selectedPort, ProcessMessageCallbackType processMessage);
	void QueueMsg(BYTE type, BYTE* msgToSend, BYTE length);
	void Update(double elapsedMS);
	void ReceivedMessage(BYTE type, BYTE* data, BYTE len);

	// Counters 
	int TimeoutCounter;

	// Serial Comm
	CSerialComm serialPortComm;

private:
	// State Machine Data
	enum EMgrState { IDLE, WAIT_RESPONSE };
	EMgrState State;

	double CommandTimoutTimerMS;
	const double COMMANDTIMEOUTMS = 500; // [miliseconds]
	SMsg MsgInExecution; // Msg in progress, use for retry!
	bool ValidResponseReceivedFlag;

	// Callback
	ProcessMessageCallbackType ProcessMessageCallback;

	// queue
	std::queue<SMsg> MsgQueue;

	// instance
	static CCommMgr* instance;
};

