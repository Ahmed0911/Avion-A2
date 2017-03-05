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
        private Comm433MHz.ReceivedMessageDelegate ReceivedMessage;

        public void Open(string portName, int baudRate, Comm433MHz.ReceivedMessageDelegate receivedMessage)
        {
            // close first
            Close();

            // Open new port
            serialPort = new SerialPort(portName, baudRate);
            serialPort.Open();

            // create new comm433MHz object
            comm433MHz = new Comm433MHz();

            // set callback
            ReceivedMessage = receivedMessage;
        }

        public void Update()
        {
            if (!IsOpen()) return;

            // Get data from serial
            int dataLen = serialPort.BytesToRead;
            byte[] buffer = new byte[dataLen];
            serialPort.Read(buffer, 0, dataLen);

            // Process Command
            comm433MHz.NewRXPacket(buffer, dataLen, ReceivedMessage);
        }

        public void Send(byte type, byte[] bufferToSend)
        {
            // Send
            byte[] outputPacket = new byte[100];
            int size = comm433MHz.GenerateTXPacket(type, bufferToSend, (byte)bufferToSend.Length, outputPacket);
            serialPort.Write(outputPacket, 0, size);
        }

        public void Close()
        {
            if (serialPort != null) serialPort.Close();
        }

        public bool IsOpen()
        {
            return (serialPort != null && serialPort.IsOpen);
        }
    }
}
