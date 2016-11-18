using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinEthApp
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            // parse data (command line parsing)
            if (args.Length > 0)
            {
                if (args[0].Contains("LogWifi"))
                {
                    MainSystem.ExtractLogFile(args[0]);
                }
                if (args[0].Contains("LogHopeRF"))
                {
                    MainSystem.ExtractLogFileRF(args[0]);
                }
                return;
            }
            //ExtractLogFile(@"C:\Users\Ivan\Desktop\DTW\Projects\Avion\A2\PCApp\A2App\bin\Debug\Log\LogWifi-19-12-51.bin");
            //ExtractLogFileRF(@"C:\Users\Ivan\Desktop\DTW\Projects\Avion\A2\Letovi\LogHopeRF-17-54-2.bin");
            //return;

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new FormMain());
        }
    }
}
