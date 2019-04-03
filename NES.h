
#ifndef NES_H_
#define NES_H_

#include "NES_ROM.h"
#include "NES_CPU.h"

class NES {
public:
	NES_ROM rom;
	NES_CPU cpu;

	bool init(char* romPath);
	bool run();
};


#endif /* NES_H_ */
