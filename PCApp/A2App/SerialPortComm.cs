using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{   
    class SerialPortComm
    {
        private SerialPort serialPort;
        public  Comm433MHz comm433MHz;

        // Serial Comm helper
        public string[] EnumeratePorts()
        {
            return SerialPort.GetPortNames();
        }

        public void Open(string portName, int baudRate)
        {
            // close first
            Close();

            // Open new port
            serialPort = new SerialPort(portName, baudRate);
            serialPort.Open();

            // create new comm433MHz object
            comm433MHz = new Comm433MHz();
        }

        public void Update(Comm433MHz.ProcessMessageDelegate ProcessMessage)
        {
            if (serialPort == null || !serialPort.IsOpen) return;

            // Get data from serial
            int dataLen = serialPort.BytesToRead;
            byte[] buffer = new byte[dataLen];
            serialPort.Read(buffer, 0, dataLen);

            // Process Command
            comm433MHz.NewRXPacket(buffer, dataLen, ProcessMessage);
        }


        public void Close()
        {
            if (serialPort != null) serialPort.Close();
        }
    }
}
