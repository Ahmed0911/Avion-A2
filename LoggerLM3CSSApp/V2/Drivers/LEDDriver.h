#ifndef LEDDRIVER_H_
#define LEDDRIVER_H_

class LEDDriver
{
public:
	enum ELED { LEDGREEN };
	
public:
	void Init();
	void Set(ELED led);
	void Reset(ELED led);
};

#endif /*LEDDRIVER_H_*/
