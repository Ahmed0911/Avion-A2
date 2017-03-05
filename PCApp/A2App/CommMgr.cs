using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    class CommMgr
    {
        // Process Delegate Callback
        public delegate void ProcessMessageDelegate(byte type, byte[] data, byte len);
        ProcessMessageDelegate ProcessMessage;

        // MSG QUEUE
        struct SMsg
        {
            public byte Type;
            public byte[] Data;
        }
        private Queue<SMsg> MsgQueue = new Queue<SMsg>();
        
        // Serial Comm
        public SerialPortComm serialPortComm = new SerialPortComm();

        // State machine
        private enum EMgrState { IDLE, WAIT_RESPONSE };
        private EMgrState State;
        private double CommandTimoutTimerMS;
        private const double COMMANDTIMEOUTMS = 500; // [miliseconds]

        private SMsg MsgInExecution; // Msg in progress, use for retry!
        private bool ValidResponseReceivedFlag;

        // Counters 
        public int TimeoutCounter;

        public void Open(string selectedPort, ProcessMessageDelegate processMessage)
        {
            serialPortComm.Open(selectedPort, 115200, ReceivedMessage);
            State = EMgrState.IDLE;
            TimeoutCounter = 0;

            ProcessMessage = processMessage;
        }

        public void QueueMsg(byte type, byte[] msgToSend)
        {
            // Add MSG to queue, schedule for execution
            SMsg msg = new SMsg();
            msg.Type = type;
            msg.Data = msgToSend;

            MsgQueue.Enqueue(msg);
        }

        public void Update(double elapsedMS)
        {
            if (!serialPortComm.IsOpen()) return;
            serialPortComm.Update();

            switch (State)
            {
                case EMgrState.IDLE:
                    {
                        if (MsgQueue.Count > 0)
                        {
                            // get message and execute (do not remove, will be removed when execution is successful)                          
                            MsgInExecution = MsgQueue.Peek();

                            // execute
                            serialPortComm.Send(MsgInExecution.Type, MsgInExecution.Data);

                            CommandTimoutTimerMS = COMMANDTIMEOUTMS;
                            ValidResponseReceivedFlag = false; // wait for response
                            State = EMgrState.WAIT_RESPONSE;
                        }
                        else
                        {
                            // queue empty, inject PING command!
                            SMsg msg = new SMsg();
                            msg.Type = 0x10; // PING command
                            msg.Data = new byte[] { 0 };
                            MsgQueue.Enqueue(msg);
                        }
                        break;
                    }

                case EMgrState.WAIT_RESPONSE:
                    {
                        if(ValidResponseReceivedFlag)
                        {
                            // got response, remove message, go IDLE
                            MsgQueue.Dequeue();
                            State = EMgrState.IDLE;
                        }
                        else
                        {
                            // check timout
                            CommandTimoutTimerMS -= elapsedMS;
                            if(CommandTimoutTimerMS < 0)
                            {
                                TimeoutCounter++;

                                // resend command, go idle without removing from queue
                                State = EMgrState.IDLE;                                
                            }
                        }
                       
                        break;
                    }
            }

        }

        public void ReceivedMessage(byte type, byte[] data, byte len)
        {
            // check if this is response
            if(State == EMgrState.WAIT_RESPONSE)
            {
                if (MsgInExecution.Type == 0x10 )
                {
                    // ping requires DATA response!
                    if(type == 0x20)
                    {
                        // GOT IT
                        ValidResponseReceivedFlag = true;
                    }
                }
                else if(type == 0xA0)
                {
                    // ACK, check type
                    if( data[0] == MsgInExecution.Type)
                    {
                        // VALID
                        ValidResponseReceivedFlag = true;
                    }
                }
            }

            // execute ProcessMessage
            ProcessMessage(type, data, len);
        }        
    }
}
