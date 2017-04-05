
#ifndef TIMER_H_
#define TIMER_H_

class Timer
{
public:
	void Init();
	void Start();
	float GetMS();
	float GetMSAndReset(); //return elapsed time in msec

private:
	unsigned long LastCount;
};

#endif /* TIMER_H_ */
