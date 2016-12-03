#include "LLConverter.h"
#include "math.h"

LLConverter::LLConverter()
{
	m_HomeSet = false;
	m_dNFactor = 0;
	m_dEFactor = 0;
	m_HomeLatitude = 0;
	m_HomeLongitude = 0;
}

bool LLConverter::IsHomeSet()
{
	return m_HomeSet;
}

void LLConverter::SetHome(double homeLatitude, double homeLongitude)
{
	// set home
	m_HomeLatitude = homeLatitude;
	m_HomeLongitude = homeLongitude;

	// calculate factors
	double R = 6378137; // equatorial radius
	double f = 1 / 298.257223563; // flattening of the planet

	double RN = R / (sqrt(1 - (2 * f - f*f)*pow(sin(m_HomeLatitude*M_PI / 180), 2)));
	double RM = RN * (1 - (2 * f - f*f)) / (1 - (2 * f - f*f)*pow(sin(m_HomeLatitude*M_PI / 180), 2));
	m_dNFactor = ((M_PI / 180) / (atan2(1, RM)));
	m_dEFactor = ((M_PI / 180) / (atan2(1, (RN*cos(m_HomeLatitude*M_PI / 180)))));

	m_HomeSet = true;
}

void LLConverter::GetHome(double& homeLatitude, double& homeLongitude)
{
	homeLatitude = m_HomeLatitude;
	homeLongitude = m_HomeLongitude;
}

void LLConverter::ConvertLLToM(double latitude, double longitude, float& N, float& E)
{
	double dLat = latitude - m_HomeLatitude; // u
	double dLong = longitude - m_HomeLongitude; // l

	double dN = dLat*m_dNFactor; // [M]
	double dE = dLong*m_dEFactor; // [M]

	N = (float)dN;
	E = (float)dE;
}

void LLConverter::ConvertMToLL(double& latitude, double& longitude, float N, float E)
{
	double dLatitude = N / m_dNFactor;
	double dLongitude = E / m_dEFactor;
	latitude = dLatitude + m_HomeLatitude;
	longitude = dLongitude + m_HomeLongitude;
}
