using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    class LLConverter
    {

        double m_HomeLatitude;
	    double m_HomeLongitude;

	    double m_dNFactor;
	    double m_dEFactor;

	    bool m_HomeSet;

        public LLConverter()
        {
	        m_HomeSet = false;
	        m_dNFactor = 0;
	        m_dEFactor = 0;
	        m_HomeLatitude = 0;
	        m_HomeLongitude = 0;
        }

        public bool IsHomeSet()
        {
	        return m_HomeSet;
        }

        public void SetHome(double homeLatitude, double homeLongitude)
        {
	        // set home
	        m_HomeLatitude = homeLatitude;
	        m_HomeLongitude = homeLongitude;

	        // calculate factors
	        double R = 6378137; // equatorial radius
	        double f = 1 / 298.257223563; // flattening of the planet

	        double RN = R / (Math.Sqrt(1 - (2 * f - f*f)*Math.Pow(Math.Sin(m_HomeLatitude*Math.PI / 180), 2)));
	        double RM = RN * (1 - (2 * f - f*f)) / (1 - (2 * f - f*f)*Math.Pow(Math.Sin(m_HomeLatitude*Math.PI / 180), 2));
	        m_dNFactor = ((Math.PI / 180) / (Math.Atan2(1, RM)));
	        m_dEFactor = ((Math.PI / 180) / (Math.Atan2(1, (RN*Math.Cos(m_HomeLatitude*Math.PI / 180)))));

	        m_HomeSet = true;
        }

        public void ConvertLLToM(double latitude, double longitude, out float N, out float E)
        {
	        double dLat = latitude - m_HomeLatitude; // u
	        double dLong = longitude - m_HomeLongitude; // l

	        double dN = dLat*m_dNFactor; // [M]
	        double dE = dLong*m_dEFactor; // [M]

	        N = (float)dN;
	        E = (float)dE;
        }

        public void ConvertMToLL(out double latitude, out double longitude, float N, float E)
        {
	        double dLatitude = N / m_dNFactor;
	        double dLongitude = E / m_dEFactor;
	        latitude = dLatitude + m_HomeLatitude;
	        longitude = dLongitude + m_HomeLongitude;
        }

    }
}
