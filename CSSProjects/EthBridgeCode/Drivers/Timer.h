/*
 * Timer.h
 *
 *  Created on: Jul 3, 2014
 *      Author: User
 */

#ifndef TIMER_H_
#define TIMER_H_

class Timer
{
public:
	static void Init();
	static unsigned int CountsPerSec;

public:
	void Start();
	float GetUS(); //return elapsed time in usec

private:
	unsigned int LastCount;
};

#endif /* TIMER_H_ */
