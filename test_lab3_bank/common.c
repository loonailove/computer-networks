#include "common.h"

uint8_t simple_csum(uint8_t *buf, size_t len) {

	/* TODO 1.1: Implement the simple checksum algorithm */
	uint8_t sum = 0;

	for (int i = 0; i < len; i++) {
		sum += buf[i];
	}

	return sum % 256;
}

uint32_t crc32(uint8_t *buf, size_t len)
{
	/* TODO 2.1: Implement the CRC 32 algorithm */

	/* Iterate through each byte of buff */
	
		/* Iterate through each bit */
		/* If the bit is 1, compute the new reminder */

	/* By convention, we negate the crc */
	uint32_t crc = ~0;	// 0xffffffff -> 11111...11
	const uint32_t POLY = 0xEDB88320; // CRC-32 checksum (IEE 802.3)

	while(len--) {
		crc = crc ^ *buf++;
		/* ^ cycling/iterating through our buffer byte by byte */
		for (int bit = 0; bit < 8; bit++) {
			if (crc & 1)
				crc = (crc >> 1) ^ POLY;
			else crc = (crc >> 1);
		}
	}

	crc = ~crc;	// prin conventie, facem flip la toti biti
	return crc;
	return 0;
}
