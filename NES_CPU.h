
#ifndef NES_CPU_H_
#define NES_CPU_H_

#include "NES_ROM.h"

class NES_CPU {
public:

	uint16_t PC;
	uint8_t SP;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t P;
	uint8_t* memory;

	NES_ROM* rom;

	void init(NES_ROM* rom);

	void pushPCtoStack();

	void retrievePCfromStack();

	uint8_t runOp();

	uint8_t AND();

	uint8_t ASL();

	uint8_t BEQ();

	uint8_t BRK();

	uint8_t INZ(uint8_t* Z);

	uint8_t LDZ(uint8_t* Z);

	uint8_t LSR();

	uint8_t SBC();

	uint8_t STA();


	inline void setCarryFlag(bool value);
	inline void setZeroFlag(bool value);
	inline void setInterruptDisable(bool value);
	inline void setDecimalMode(bool value);
	inline void setBRK(bool value);
	inline void setOverflow(bool value);
	inline void setNegative(bool value);

	inline bool isSetCarryFlag();
	inline bool isSetZeroFlag();
	inline bool isSetInterruptDisable();
	inline bool isSetDecimalMode();
	inline bool isSetBRK();
	inline bool isSetOverflow();
	inline bool isSetNegative();


	void d_printMemFromPC();
};



#endif /* NES_CPU_H_ */
