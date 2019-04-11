
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

	inline void pushToStack(uint8_t value);

	inline uint8_t pullFromStack();

	uint8_t runOp();

	uint8_t ADC();

	uint8_t AND();

	uint8_t ASL();

	uint8_t BENQ(bool value);

	uint8_t BIT();

	uint8_t BIC(bool carrySet);

	uint8_t BIZ(bool zeroSet);

	uint8_t BRK();

	//uint8_t BVC();

	uint8_t CMP(uint8_t* Z);

	uint8_t DEZ(uint8_t* Z);

	uint8_t EOR();

	uint8_t INZ(uint8_t* Z);

	uint8_t JMP();

	uint8_t JSR();

	uint8_t LDZ(uint8_t* Z);

	uint8_t LSR();

	uint8_t ORA();

	uint8_t PLA();

	inline uint8_t RTI();

	uint8_t SBC();

	uint8_t STZ(uint8_t Z);

	uint8_t branchIfFlagSet(bool flag, bool isSet);

	inline uint8_t BCS();
	inline uint8_t BCC();
	inline uint8_t BEQ();
	inline uint8_t BNE();
	inline uint8_t BMI();
	inline uint8_t BPL();
	inline uint8_t BVC();
	inline uint8_t BVS();


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

	inline uint8_t getImmediateValue();
	inline uint8_t getZeroPageValue();
	inline uint8_t getZeroPageXValue();
	inline uint8_t getZeroPageYValue();
	inline uint8_t getAbsoluteValue();
	inline uint16_t getAbsoluteAddress();
	inline uint8_t getAbsoluteXValue();
	inline uint8_t getAbsoluteYValue();
	inline uint8_t getIndirectValue();
	inline uint8_t getIndirectXValue();
	inline uint8_t getIndirectYValue();

	inline void branchRelative();

	void d_printMemFromPC();
};



#endif /* NES_CPU_H_ */
