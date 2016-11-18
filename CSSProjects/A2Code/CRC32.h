/*
 * CRC32.h
 *
 *  Created on: Oct 29, 2016
 *      Author: Ivan
 */

#ifndef CRC32_H_
#define CRC32_H_

class CRC32
{
public:
	void Init();
	unsigned int CalculateCRC32(unsigned char *buf, int len);

};

#endif /* CRC32_H_ */
