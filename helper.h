#include "header.h"

#ifndef HELPER_H_
#define HELPER_H_

bool isBitSet(uint8_t byte, uint8_t bitpos);
void setBit(uint8_t* byte, uint8_t bitpos, bool toSet);

uint16_t combineLowHigh(uint8_t low, uint8_t high);



#endif /* HELPER_H_ */
