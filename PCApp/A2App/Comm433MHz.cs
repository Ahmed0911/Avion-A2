using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    class Comm433MHz
    {
        public delegate void ReceivedMessageDelegate(byte type, byte[] data, byte len);


        private enum ERXPhase { HDR_FF, HDR_FE, HDR_A5, HDR_5A, TYPE, LEN, DATA, CRC_A, CRC_B, CRC_C, CRC_D };
        private ERXPhase RXPhase;

        private byte Type;
        private byte Len;
        private byte[] Data = new byte[255];
        private int DataIndex = 0;
        private uint CRC;

        // counters
        public int MsgReceivedOK = 0;
        public int CrcErrors = 0;
        public int HeaderFails = 0;

        public void NewRXPacket(byte[] data, int dataLen, ReceivedMessageDelegate ReceivedMessage)
        {
            // RX Parser
            for (int i = 0; i != dataLen; i++)
            {
                byte b = data[i];
                switch (RXPhase)
                {
                    // HEADER
                    case ERXPhase.HDR_FF:
                        if (b == 0xFF)
                        {
                            RXPhase = ERXPhase.HDR_FE; // wait for start
                        }
                        break;

                    case ERXPhase.HDR_FE:
                        if (b == 0xFE)
                        {
                            RXPhase = ERXPhase.HDR_A5; // wait for start
                        }
                        else
                        {
                            RXPhase = ERXPhase.HDR_FF; // reset
                            HeaderFails++;
                        }
                        break;

                    case ERXPhase.HDR_A5:
                        if (b == 0xA5)
                        {
                            RXPhase = ERXPhase.HDR_5A; // wait for start
                        }
                        else
                        {
                            RXPhase = ERXPhase.HDR_FF; // reset
                            HeaderFails++;
                        }
                        break;

                    case ERXPhase.HDR_5A:
                        if (b == 0x5A)
                        {
                            RXPhase = ERXPhase.TYPE; // wait for start
                        }
                        else
                        {
                            RXPhase = ERXPhase.HDR_FF; // reset
                            HeaderFails++;
                        }
                        break;

                    case ERXPhase.TYPE:
                        Type = b;
                        RXPhase = ERXPhase.LEN;
                        break;

                    case ERXPhase.LEN:
                        Len = b;
                        DataIndex = 0; // reset data counter
                        RXPhase = ERXPhase.DATA;
                        break;


                    case ERXPhase.DATA:
                        Data[DataIndex] = b;
                        DataIndex++;
                        if (DataIndex >= Len) RXPhase = ERXPhase.CRC_A;
                        break;

                    case ERXPhase.CRC_A:
                        CRC = (uint)(b << 24);
                        RXPhase = ERXPhase.CRC_B;
                        break;

                    case ERXPhase.CRC_B:
                        CRC += (uint)(b << 16);
                        RXPhase = ERXPhase.CRC_C;
                        break;

                    case ERXPhase.CRC_C:
                        CRC += (uint)(b << 8);
                        RXPhase = ERXPhase.CRC_D;
                        break;

                    case ERXPhase.CRC_D:
                        CRC += b;
                        // check CRC, process data!!
                        uint calculatedCRC = Crc32.CalculateCrc32(Data, Len);
                        if(calculatedCRC == CRC)
                        {
                            // message OK, process!
                            MsgReceivedOK++;

                            ReceivedMessage(Type, Data, Len);
                        }
                        else
                        {
                            // CRC Failed
                            CrcErrors++;
                        }

                        RXPhase = ERXPhase.HDR_FF;
                        break;
                }

            }
        }

        public int GenerateTXPacket(byte Type, byte[] Data, byte Len, byte[] OutputPacket)
        {
            // assemble header
            OutputPacket[0] = 0xFF; // Header
            OutputPacket[1] = 0xFE; // Header
            OutputPacket[2] = 0xA5; // Header
            OutputPacket[3] = 0x5A; // Header

            OutputPacket[4] = Type; // Type
            OutputPacket[5] = Len; // Data Length
            for(int i=0;i!=Len; i++)
            {
                OutputPacket[6 + i] = Data[i];
            }

            // CRC
            uint calculatedCRC = Crc32.CalculateCrc32(Data, Len);
            OutputPacket[6 + Len] = (byte)(calculatedCRC >> 24);
            OutputPacket[6 + Len+1] = (byte)(calculatedCRC >> 16);
            OutputPacket[6 + Len+2] = (byte)(calculatedCRC >> 8);
            OutputPacket[6 + Len+3] = (byte)(calculatedCRC);

            return 6 + Len + 4; // return total length (4xHEDER + TYPE + LEN + 4xCHKSUM)
        }
    }
}
