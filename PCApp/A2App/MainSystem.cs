using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinEthApp
{
    class MainSystem
    {
        public FormMain formMain;

        // UDP Communication
        private const int TargetPort = 12000;
        private int LocalPort = 0; // autoselect port (will be sent in ping command)
        private string TargetAdress = "";
        private UdpClient udp = null;
        private int ReceivedPacketsCounter = 0;
        private int PingCounter = 0;

        // CommMgr
        public CommMgr commMgr = new CommMgr();

        // Assist now
        private AssistNow assistNow = new AssistNow();

        // Data
        SCommEthData MainSystemData;
        SCommHopeRFDataA2Avion RelayedData;

        // Log File
        FileStream logStream = null;
        FileStream logStreamRF = null;

        public MainSystem(FormMain form)
        {
            formMain = form;
           
            // open log file
            //string filename = string.Format("Log\\LogWifi-{0}-{1}-{2}.bin", DateTime.Now.Hour, DateTime.Now.Minute, DateTime.Now.Second);
            //logStream = File.Create(filename);
            // open RF log file
            string filenameRF = string.Format("Log\\LogHopeRF-{0}-{1}-{2}.bin", DateTime.Now.Hour, DateTime.Now.Minute, DateTime.Now.Second);
            logStreamRF = File.Create(filenameRF);
        }

        // UDP
        public void Connect(string targetAddress)
        {
            TargetAdress = targetAddress;

            udp = new UdpClient(LocalPort);
            LocalPort = ((IPEndPoint)udp.Client.LocalEndPoint).Port;
        }

        // Serial
        public void ConnectToSerial(string selectedPort)
        {
            commMgr.Open(selectedPort, ProcessMessage);            
        }

        ///////////////////////////////
        // DATA PARSER
        ///////////////////////////////
        public unsafe void Update()
        {
            ///////////////////////////////
            // Process Serial Data
            ///////////////////////////////
            commMgr.Update(10);

            ///////////////////////////////
            // Process Ethernet Data
            ///////////////////////////////
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
                            MainSystemData = commData;
                            
                            // filter data
                            formMain.FilteredRoll = 0.9998 * formMain.FilteredRoll + 0.0002 * MainSystemData.dRoll;
                            formMain.FilteredPitch = 0.9998 * formMain.FilteredPitch + 0.0002 * MainSystemData.dPitch;
                            formMain.FilteredYaw = 0.9998 * formMain.FilteredYaw + 0.0002 * MainSystemData.dYaw;

                            for (int i = 0; i != 10; i++)
                            {
                                formMain.FilteredTuningData[i] = 0.999 * formMain.FilteredTuningData[i] + 0.001 * Math.Abs(commData.TuningData[i] - formMain.FilteredTuningLastData[i]);
                                formMain.FilteredTuningLastData[i] = commData.TuningData[i];
                            }

                            logStream.Write(withoutHeader, 0, withoutHeader.Length);
                            break;

                        case 0x62: // get params data
                            SParameters p = (SParameters)Comm.FromBytes(withoutHeader, new SParameters());

                            // Display
                            formMain.DisplayParams(p);                            
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
            formMain.DisplaySystemData(ReceivedPacketsCounter, MainSystemData);

            // Update assist now
            byte[] toSend = assistNow.Update(MainSystemData.AssistNextChunkToSend);
            if (toSend != null) SendData(0x30, toSend);            
        }



        ///////////////////////////////
        // COMMANDS
        ///////////////////////////////
        public void ExecuteAssistNow()
        {
            assistNow.Execute();
        }
        public void ReadParams()
        {
            // Send read request
            byte[] toSend = new byte[1];
            formMain.SendData(0x61, toSend);
        }

        public void WriteParams(SParameters p)
        {
            // Send
            byte[] toSend = Comm.GetBytes(p);
            formMain.SendData(0x60, toSend);
        }

        public void StoreToFlashParams()
        {
            // Send flash store command
            byte[] toSend = new byte[1];
            formMain.SendData(0x63, toSend);
        }

        public void WpnCommand(byte index, byte command)
        {
            uint code = 0x43782843;
            uint timer = 400; // ticks, 1 sec
            SCommLaunch launch = new SCommLaunch();
            launch.Command = command;
            launch.Index = index;
            if (command == 2) launch.CodeTimer = timer;
            else launch.CodeTimer = code;

            // Send
            byte[] toSend = Comm.GetBytes(launch);
            SendData(0x90, toSend);
        }

        ///////////////////////////////
        // Serial Parser + Stuff
        ///////////////////////////////
        public void ProcessMessage(byte type, byte[] data, byte len)
        {
            // data
            if (type == 0x20)
            {
                SCommHopeRFDataA2Avion commDataHopeRF = (SCommHopeRFDataA2Avion)Comm.FromBytes(data, new SCommHopeRFDataA2Avion());
                
                RelayedData = commDataHopeRF;

                // save to file
                logStreamRF.Write(data, 0, data.Length);

                // filter data
                formMain.FilteredRoll = 0.99 * formMain.FilteredRoll + 0.01 * RelayedData.dRoll;
                formMain.FilteredPitch = 0.99 * formMain.FilteredPitch + 0.01 * RelayedData.dPitch;
                formMain.FilteredYaw = 0.99 * formMain.FilteredYaw + 0.01 * RelayedData.dYaw;

                formMain.DisplayRelayedData(RelayedData);
            }
            if (type == 0x62)
            {

                // Parameters structure
                SParameters parametersDataHopeRF = (SParameters)Comm.FromBytes(data, new SParameters());

                // Display
                formMain.DisplayParams(parametersDataHopeRF);
            }
        }

        public void QueueSerial(byte type, byte[] msg)
        {
            commMgr.QueueMsg(type, msg);
        }

        ///////////////////////////////
        // HELPERS
        ///////////////////////////////
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

        public static unsafe void ExtractLogFile(string filename)
        {
            // open txt file
            StreamWriter sw = File.CreateText("matlab.txt");
            // create header
            sw.Write("Loop ");
            sw.Write("Voltage FuelLevel CurrentA TotalCharge_mAh ");
            sw.Write("MagX MagY MagZ ");
            sw.Write("AccX AccY AccZ ");
            sw.Write("Roll Pitch Yaw ");
            sw.Write("dRoll dPitch dYaw ");
            sw.Write("Pressure Temperature Altitude Vertspeed ");
            sw.Write("PWM1 PWM2 PWM3 PWM4 ");
            sw.Write("NumSV VelN VelE VelD HeightMSL HorizontalAccuracy ");
            for (int i = 0; i != 10; i++)
            {
                sw.Write("D{0} ", i);
            }
            sw.WriteLine("Mode HopeRXRSSI HopeRXFrameCnt HopeRSSI ");
            sw.WriteLine("");

            SCommEthData data = new SCommEthData();
            FileStream logStream = File.OpenRead(filename);
            int size = Marshal.SizeOf(data);
            byte[] dataArray = new byte[size];
            while (logStream.Read(dataArray, 0, size) > 0)
            {
                data = (SCommEthData)Comm.FromBytes(dataArray, new SCommEthData());
                sw.Write("{0} ", data.LoopCounter);
                sw.Write("{0} {1} {2} {3} ", data.BatteryVoltage, data.FuelLevel, data.BatteryCurrentA, data.BatteryTotalCharge_mAh);
                sw.Write("{0} {1} {2} ", data.MagX, data.MagY, data.MagZ);
                sw.Write("{0} {1} {2} ", data.AccX, data.AccY, data.AccZ);
                sw.Write("{0} {1} {2} ", data.Roll, data.Pitch, data.Yaw);
                sw.Write("{0} {1} {2} ", data.dRoll, data.dPitch, data.dYaw);
                sw.Write("{0} {1} {2} {3} ", data.Pressure, data.Temperature, data.Altitude, data.Vertspeed);
                sw.Write("{0} {1} {2} {3} ", data.MotorThrusts[0], data.MotorThrusts[1], data.MotorThrusts[2], data.MotorThrusts[3]);
                sw.Write("{0} {1} {2} {3} {4} {5} ", data.NumSV, data.VelN / 1000.0, data.VelE / 1000.0, data.VelD / 1000.0, data.HeightMSL / 1000.0, data.HorizontalAccuracy / 1000.0);
                for (int i = 0; i != 10; i++ )
                {
                    sw.Write("{0} ", data.TuningData[i]);
                }
                sw.Write("{0} {1} {2} {3} ", data.ActualMode, data.HopeRXRSSI, data.HopeRXFrameCount, data.HopeRSSI);
                sw.WriteLine("");
            }

            sw.Close();
        }

        public static unsafe void ExtractLogFileRF(string filename)
        {
            // open txt file
            StreamWriter sw = File.CreateText("matlabRF.txt");
            // create header
            sw.Write("Loop ");
            sw.Write("Voltage CurrentA TotalCharge_mAh ");
            sw.Write("Roll Pitch Yaw ");
            sw.Write("dRoll dPitch dYaw ");
            sw.Write("Altitude Vertspeed ");
            sw.Write("NumSV VelN VelE ");     
            sw.Write("HopeRXRSSI HopeTXRSSI ");
            sw.WriteLine("PWM1 PWM2 PWM3 PWM4 ");

            SCommHopeRFDataA2Avion data = new SCommHopeRFDataA2Avion();
            FileStream logStream = File.OpenRead(filename);
            int size = Marshal.SizeOf(data);
            byte[] dataArray = new byte[size];
            while (logStream.Read(dataArray, 0, size) > 0)
            {
                data = (SCommHopeRFDataA2Avion)Comm.FromBytes(dataArray, new SCommHopeRFDataA2Avion());
                sw.Write("{0} ", data.LoopCounter);
                sw.Write("{0} {1} {2} ", data.BatteryVoltage, data.BatteryCurrentA, data.BatteryTotalCharge_mAh);
                sw.Write("{0} {1} {2} ", data.Roll, data.Pitch, data.Yaw);
                sw.Write("{0} {1} {2} ", data.dRoll, data.dPitch, data.dYaw);
                sw.Write("{0} {1} ", data.Altitude, data.Vertspeed);
                sw.Write("{0} {1} {2} ", data.NumSV, data.VelN / 1000.0, data.VelE / 1000.0);
                sw.Write("{0} {1} ", data.HopeRXRSSI, data.HopeTXRSSI);
                sw.Write("{0} {1} {2} {3} ", data.MotorThrusts[0], data.MotorThrusts[1], data.MotorThrusts[2], data.MotorThrusts[3]);
                sw.WriteLine("");
            }

            sw.Close();
        }
    }
}
