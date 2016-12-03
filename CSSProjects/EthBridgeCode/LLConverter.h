#pragma once

#define M_PI 3.1415926535897932384

class LLConverter
{
public:
	LLConverter();

	void SetHome(double homeLatitude, double homeLongitude);
	void GetHome(double& homeLatitude, double& homeLongitude);
	bool IsHomeSet();

	void ConvertLLToM(double latitude, double longitude, float& N, float& E);
	void ConvertMToLL(double& latitude, double& longitude, float N, float E);

private:
	double m_HomeLatitude;
	double m_HomeLongitude;

	double m_dNFactor;
	double m_dEFactor;

	bool m_HomeSet;
};

