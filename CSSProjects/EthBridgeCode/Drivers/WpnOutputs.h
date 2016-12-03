/*
 * WpnOutputs.h
 *
 *  Created on: Jun 26, 2015
 *      Author: Sara
 */

#ifndef WPNOUTPUTS_H_
#define WPNOUTPUTS_H_

#define WPNOUT1 1
#define WPNOUT2 2

class WpnOutputs
{
public:
	void Init();
	void Set(int out);
	void Set(int out, bool set);
	void Reset(int out);
};

#endif /* WPNOUTPUTS_H_ */
