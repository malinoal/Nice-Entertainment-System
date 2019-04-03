
#ifndef NES_ROM_H_
#define NES_ROM_H_

class NES_ROM {
public:
	uint8_t* romContents; //TODO: Implement free function
	std::streampos size;
	uint8_t prg_banks;
	uint8_t chr_banks;
	uint8_t mirrortype; //0=horizontal, 1=vertical, 2=fourscreen
	bool batteryRamPresent;
	bool trainerPresent;
	uint8_t ramBanks;
	uint8_t* prg_rom;
	uint8_t* chr_rom;

	bool loadRom(char* romPath);
	void d_printRom();
	void d_printPRG();
};



#endif /* NES_ROM_H_ */
