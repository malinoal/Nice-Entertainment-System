#include "header.h"

#include "NES.h"

bool NES::init(char* romPath) {
	if(!rom.loadRom(romPath)) return false;
	cpu.init(&rom);

	return true;
}

bool NES::run() {
	uint8_t cycles = cpu.runOp();
	printf("CPU ran for %i cycles\n", cycles);
	if(cycles < 1) return false;
	else return true;
}



