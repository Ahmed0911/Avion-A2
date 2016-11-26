#pragma once
class CRC32
{
public:
	static unsigned int digital_update_crc32(unsigned int crc, BYTE* data, int len);
	static unsigned int CalculateCrc32(BYTE* buf, int size);
};

