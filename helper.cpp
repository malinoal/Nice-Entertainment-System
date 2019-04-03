#include "helper.h"

bool isBitSet(uint8_t byte, uint8_t bitpos) { //Unit Tested
	uint8_t bit = 1 << bitpos;

	bit = bit & byte;

	return bit;
}

void setBit(uint8_t* byte, uint8_t bitpos, bool toSet) { //Unit Tested
	uint8_t bit = 1 <<bitpos;

	if(toSet)
		*byte |= bit;
	else
		if(isBitSet(*byte, bitpos)) *byte ^= bit;
}


uint16_t combineLowHigh(uint8_t low, uint8_t high) { //Unit Tested
	uint16_t result = 0;
	result = low;
	result = result | (high << 8);

	return result;
}





