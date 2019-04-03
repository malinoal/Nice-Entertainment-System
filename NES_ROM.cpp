#include "header.h"

#include "NES_ROM.h"
#include "helper.h"


bool NES_ROM::loadRom(char* romPath) {

	std::ifstream rom (romPath, std::ios::in | std::ios::binary | std::ios::ate);

	if(rom.is_open()) {
		size = rom.tellg();

		romContents = new uint8_t[size]();

		rom.seekg(0, std::ios::beg);

		rom.read((char*)romContents, size);

		rom.close();

		if(romContents[3] != 0x1a) {
			printf("ERROR: selected ROM is invalid\n");
			return false;
		}

		prg_banks = romContents[4];
		chr_banks = romContents[5];

		mirrortype = isBitSet(romContents[6], 0);
		batteryRamPresent = isBitSet(romContents[6], 1);
		trainerPresent = isBitSet(romContents[6], 2);
		if(isBitSet(romContents[6], 3)) mirrortype = 2;
		//TODO: Mapper Number
		ramBanks = romContents[8] == 0 ? 1 : romContents[8];

		prg_rom = trainerPresent ? &romContents[512+16] : &romContents[16];
		chr_rom = &prg_rom[prg_banks*KB16];

		return true;
	} else {
		printf("ERROR: Rom at %s could not be opened\n", romPath);
		return false;
	}
} //end loadRom

void NES_ROM::d_printRom() {
	if(romContents != NULL) {
		printf("Dumping the first KB of ROM: \n");
		for(int i = 0; i < 1024; ++i) {
			for(int j = 0; j++ < 16; ++i) {
				printf("%02x ", romContents[i]);
			}
			printf("\n");
			i--;
		}
	}
}

void NES_ROM::d_printPRG() {
	printf("Dumping the first KB of PRG, located in memory at either %04x or %04x\n", 10, 522);
	for(int i = 0; i < 1024; ++i) {
		for(int j = 0; j++ < 16; ++i) {
			printf("%02x ", prg_rom[i]);
		}
		printf("\n");
		i--;
	}
}

