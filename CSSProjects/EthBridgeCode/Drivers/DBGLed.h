/*
 * DBGLed.h
 *
 *  Created on: Jun 26, 2015
 *      Author: Sara
 */

#ifndef DBGLED_H_
#define DBGLED_H_

class DBGLed
{
private:
	bool m_LastState;

public:
	void Init();
	void Set(bool set);
	void Set();
	void Reset();
	void Toggle();
};

#endif /* DBGLED_H_ */
