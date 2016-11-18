using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    class AssistNow
    {
        private enum eAssistPhase { IDLE, START, SENDCHUNK, WAIT, END, FAIL };
        private byte[] AssistArray;
        private eAssistPhase assistPhase = eAssistPhase.IDLE;
        private int ChunkIndex;
        private int RetryTimer;
        private int RetryCounter;

        private const int BYTESPERCHUNK = 500; // 500 bytes per chunk
        private const int RETRYTIMERCOUNT = (int)(20 * 0.5); // 0.5 sec
        private const int RETRYCOUNTERCOUNT = 5; // 5 retries per chunk        

        public void Execute()
        {
            // Create a request for the URL. 
            //string requestStr = "http://online-live1.services.u-blox.com/GetOnlineData.ashx?token=hKeEq-6GYkOIoPGNWt1z9w;gnss=gps,glo;datatype=alm";
            string requestStr = "http://online-live1.services.u-blox.com/GetOnlineData.ashx?token=hKeEq-6GYkOIoPGNWt1z9w;gnss=gps,glo;datatype=eph,alm";
            WebRequest request = WebRequest.Create(requestStr);
            // Get the response.
            WebResponse response = request.GetResponse();
            string status = ((HttpWebResponse)response).StatusDescription;
            Stream dataStream = response.GetResponseStream();

            // convert to byte array            
            MemoryStream ms = new MemoryStream();
            dataStream.CopyTo(ms);
            byte[] array = ms.ToArray();
            ms.Close();
            response.Close();

            // TEST
            //FileStream f = new FileStream("miki.bin", FileMode.Create);
            //f.Write(array, 0, array.Length);
            //f.Close();

            AssistArray = array;
            assistPhase = eAssistPhase.START;
        }

        public byte[] Update(int nextChunkToSend)
        {
            byte[] toSend = null;

            switch (assistPhase)
            {
                case eAssistPhase.IDLE:
                    // do nothing
                    break;

                case eAssistPhase.START:
                    // Init transfer
                    ChunkIndex = 0;
                    RetryCounter = RETRYCOUNTERCOUNT;
                    assistPhase = eAssistPhase.SENDCHUNK;
                    break;

                case eAssistPhase.SENDCHUNK:
                    // send chunk
                    int numOfChunks = (int)Math.Ceiling((double)AssistArray.Length / BYTESPERCHUNK);
                    if (ChunkIndex < numOfChunks)
                    {
                        toSend = SendChunk(ChunkIndex);
                        RetryTimer = RETRYTIMERCOUNT;
                        assistPhase = eAssistPhase.WAIT;
                    }
                    else
                    {
                        // DONE
                        assistPhase = eAssistPhase.END;
                    }
                    break;

                case eAssistPhase.WAIT:
                    if(nextChunkToSend == (ChunkIndex+1)) // chunk processed?
                    {
                        // send next
                        ChunkIndex++;
                        RetryCounter = RETRYCOUNTERCOUNT;
                        assistPhase = eAssistPhase.SENDCHUNK;
                    }
                    else if( --RetryTimer <= 0)
                    {
                        RetryCounter--;
                        if (RetryCounter >= 0)
                        {
                            // resend
                            assistPhase = eAssistPhase.SENDCHUNK;
                        }
                        else
                        {
                            // error/fail
                            assistPhase = eAssistPhase.FAIL;
                        }
                    }
                    break;

                case eAssistPhase.END:
                    // DONE
                    break;
                

                case eAssistPhase.FAIL:
                    // error
                    break;

            }

            return toSend;
        }

        private byte[] SendChunk(int chunkIndex)
        {            
            int index = chunkIndex * BYTESPERCHUNK;
            int size = Math.Min((AssistArray.Length - index), BYTESPERCHUNK);

            byte[] ret = new byte[size];
            Array.Copy(AssistArray, index, ret, 0, size);

            return ret;
        }
    }
}
