/*
 * BaroDrv.h
 *
 *  Created on: Jun 30, 2015
 *      Author: Sara
 */

#ifndef BARODRV_H_
#define BARODRV_H_

class BaroDrv
{
public:
	void Init();
	void Update();

	float PressurePa;
	float TemperatureC;

private:
	void Reset();
	unsigned short ReadProm(unsigned char address);
	void StartConversion(unsigned char D12Command);
	unsigned int ReadADC();

	unsigned short C1, C2, C3, C4, C5, C6;
	bool m_ReadTemperaturePhase;
	unsigned int D1, D2;
};

#endif /* BARODRV_H_ */
