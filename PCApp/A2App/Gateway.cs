using System; 
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    class Gateway
    {
        private FormMain formMain;

        // UDP Communication
        private const int TargetPort = 12000;
        private int LocalPort = 0; // autoselect port (will be sent in ping command)
        private string TargetAdress = "";
        private UdpClient udp = null;
        private int ReceivedPacketsCounter = 0;
        private int PingCounter = 0;

        private int ReceivedHopeRFCounter = 0;
        private int ReceivedHopeRFCounterCrcErrors = 0;

        // Data
        SCommEthData GatewayData;
        SCommHopeRFDataA2Avion RelayedData;

        // Log File
        FileStream logStream = null;

        public Gateway(FormMain formMain)
        {
            this.formMain = formMain;

            // open log file
            string filename = string.Format("Log\\LogHopeRF-{0}-{1}-{2}.bin", DateTime.Now.Hour, DateTime.Now.Minute, DateTime.Now.Second);
            logStream = File.Create(filename);
        }

        public void Connect(string targetAddress)
        {
            TargetAdress = targetAddress;

            udp = new UdpClient(LocalPort);
            LocalPort = ((IPEndPoint)udp.Client.LocalEndPoint).Port;
        }

        public void Update()
        {
            if (udp == null) return;

            // Get data from UDP
            while (udp.Available > 0)
            {
                // get data
                IPEndPoint endP = new IPEndPoint(IPAddress.Any, 0); // any port of the sender
                byte[] bytes = udp.Receive(ref endP);

                // process data
                ReceivedPacketsCounter++;
                if (PacketIsValid(bytes))
                {
                    // remove header
                    byte[] withoutHeader = bytes.Skip(3).ToArray();
                    switch (bytes[2])
                    {
                        case 0x20:
                            // data                            
                            SCommEthData commData = (SCommEthData)Comm.FromBytes(withoutHeader, new SCommEthData());
                            GatewayData = commData;
                            break;

                        case 0x41:
                            // relayed from RF
                            if (withoutHeader.Length == Marshal.SizeOf(new SCommHopeRFDataA2Avion())) // check size
                            {
                                // DataA2 structure
                                ReceivedHopeRFCounter++;

                                SCommHopeRFDataA2Avion commDataHopeRF = (SCommHopeRFDataA2Avion)Comm.FromBytes(withoutHeader, new SCommHopeRFDataA2Avion());
                                commDataHopeRF.HopeTXRSSI = GatewayData.HopeRXRSSI; // fill with received/gatewayed Hope RSSI

                                // check checksum
                                uint crcSum = Crc32.CalculateCrc32(withoutHeader, withoutHeader.Length - sizeof(uint) );                                
                                if (crcSum == commDataHopeRF.CRC32) // CRC OK?
                                {                                    
                                    RelayedData = commDataHopeRF;

                                    // save to file
                                    byte[] arrayToWrite = Comm.GetBytes(commDataHopeRF);
                                    logStream.Write(arrayToWrite, 0, arrayToWrite.Length);
                                }
                                else ReceivedHopeRFCounterCrcErrors++;                                
                            }
                            else if( withoutHeader.Length == Marshal.SizeOf(new SParameters())) // check size
                            {
                                // Parameters structure
                                 SParameters parametersDataHopeRF = (SParameters)Comm.FromBytes(withoutHeader, new SParameters());
                                
                                // check checksum
                                uint crcSum = Crc32.CalculateCrc32(withoutHeader, withoutHeader.Length - sizeof(uint) );                                
                                if (crcSum == parametersDataHopeRF.CRC32) // CRC OK?
                                {
                                    // Display
                                    formMain.DisplayParams(parametersDataHopeRF);  
                                }

                            }
                            break;                       
                    }
                }

            }

            // send PING
            if (++PingCounter > 20)
            {
                PingCounter = 0;
                byte[] lp = BitConverter.GetBytes(LocalPort); // send port
                SendData(0x10, lp);
            }

            // GUI update
            formMain.DisplayGatewayData(ReceivedHopeRFCounter, ReceivedHopeRFCounterCrcErrors, ReceivedPacketsCounter, GatewayData);
            formMain.DisplayRelayedData(RelayedData);                 
        }

        private bool PacketIsValid(byte[] bytes)
        {
            bool valid = true;
            if (bytes[0] != 0x42) valid = false;
            if (bytes[1] != 0x24) valid = false;

            return valid;
        }

        public void SendData(byte type, byte[] buffer)
        {
            if (udp == null) return;

            byte[] data = new byte[buffer.Length + 3];
            // assemble
            data[0] = 0x42;
            data[1] = 0x24;
            data[2] = type; // Type
            Array.Copy(buffer, 0, data, 3, buffer.Length);
            udp.Send(data, data.Length, TargetAdress, TargetPort);
        }

        public void SendDataOverRF(byte type, byte[] buffer)
        {
            if (udp == null) return;

            byte[] data = new byte[buffer.Length + 4];
            // assemble
            data[0] = 0x42;
            data[1] = 0x24;
            data[2] = 0x40; // Relay Data Over RF
            data[3] = type; // Type
            Array.Copy(buffer, 0, data, 4, buffer.Length);
            udp.Send(data, data.Length, TargetAdress, TargetPort);
        }

        // Tracker Commands
        public void ChangeTrackerMode(int trackingMode, int panRef, int tiltRef)
        {
            SCommTrackerCommands tracCmd = new SCommTrackerCommands();
            tracCmd.Mode = trackingMode;
            tracCmd.PanRef = panRef;
            tracCmd.TiltRef = tiltRef;

            // Send
            byte[] toSend = Comm.GetBytes(tracCmd);
            SendData(0xA0, toSend);
        }

        public void SendTrackerRef(int panRef, int tiltRef)
        {
            SCommTrackerCommands tracCmd = new SCommTrackerCommands();
            tracCmd.Mode = 1; // manual mode!
            tracCmd.PanRef = panRef;
            tracCmd.TiltRef = tiltRef;

            //Debug.WriteLine("Pan {0}, Tilt {1}", tracCmd.PanRef, tracCmd.TiltRef);

            // Send
            byte[] toSend = Comm.GetBytes(tracCmd);
            SendData(0xA0, toSend);
        }
    }
}
