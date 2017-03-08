#include "stdafx.h"
#include "CommMgr.h"
#include <string.h>

// Callback wrapper
void ReceivedMessageWrapper(BYTE type, BYTE* data, BYTE len)
{
	CCommMgr::getInstance()->ReceivedMessage(type, data, len);
}

CCommMgr* CCommMgr::instance;
CCommMgr* CCommMgr::getInstance()
{
	return CCommMgr::instance;
}

CCommMgr::CCommMgr()
{
	instance = this;
}

void CCommMgr::Open(TCHAR* selectedPort, ProcessMessageCallbackType processMessage)
{
	serialPortComm.ConnectTo(selectedPort, ReceivedMessageWrapper);
	State = CCommMgr::IDLE;
	TimeoutCounter = 0;

	ProcessMessageCallback = processMessage;
}

void CCommMgr::QueueMsg(BYTE type, BYTE* msgToSend, BYTE length)
{
	// Add MSG to queue, schedule for execution
	SMsg msg;
	msg.Type = type;
	memcpy(msg.Data, msgToSend, length);
	msg.Length = length;

	MsgQueue.push(msg);
}

void CCommMgr::Update(double elapsedMS)
{
	if (!serialPortComm.IsOpen() ) return;
	serialPortComm.Update();

	switch (State)
	{
	case CCommMgr::IDLE:
	{
		if (!MsgQueue.empty())
		{
			// get message and execute (do not remove, will be removed when execution is successful)                          
			MsgInExecution = MsgQueue.front();

			// execute
			serialPortComm.SendData(MsgInExecution.Type, MsgInExecution.Data, MsgInExecution.Length);

			CommandTimoutTimerMS = COMMANDTIMEOUTMS;
			ValidResponseReceivedFlag = false; // wait for response
			State = CCommMgr::WAIT_RESPONSE;
		}
		else
		{
			// queue empty, inject PING command!
			SMsg msg;
			msg.Type = 0x10; // PING command
			msg.Data[0] = 0;
			msg.Length = 1;
			MsgQueue.push(msg);
		}
		break;
	}

	case CCommMgr::WAIT_RESPONSE:
	{
		if (ValidResponseReceivedFlag)
		{
			// got response, remove message, go IDLE
			MsgQueue.pop();
			State = CCommMgr::IDLE;
		}
		else
		{
			// check timout
			CommandTimoutTimerMS -= elapsedMS;
			if (CommandTimoutTimerMS < 0)
			{
				TimeoutCounter++;

				// resend command, go idle without removing from queue
				State = CCommMgr::IDLE;
			}
		}

		break;
	}
	}

}

void CCommMgr::ReceivedMessage(BYTE type, BYTE* data, BYTE len)
{
	// check if this is response
	if (State == CCommMgr::WAIT_RESPONSE)
	{
		if (MsgInExecution.Type == 0x10)
		{
			// ping requires DATA response!
			if (type == 0x20)
			{
				// GOT IT
				ValidResponseReceivedFlag = true;
			}
		}
		else if (type == 0xA0)
		{
			// ACK, check type
			if (data[0] == MsgInExecution.Type)
			{
				// VALID
				ValidResponseReceivedFlag = true;
			}
		}
	}

	// execute ProcessMessage
	ProcessMessageCallback(type, data, len);
}