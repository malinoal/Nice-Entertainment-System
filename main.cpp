#include "header.h"
#include "NES.h"


int main(int argc, char* args[]) {

	NES emu;

	emu.init(args[1]);

	while(emu.run()){}

	//emu.cpu.d_printMemFromPC();

}

