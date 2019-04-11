#include "header.h"

#include "NES_CPU.h"
#include "helper.h"

#define CPU_DEBUG 1

#define NESTEST 1

#if CPU_DEBUG
	int d_totalInstructions = 0;
#endif


void NES_CPU::init(NES_ROM* _rom) {
	memory = new uint8_t[0x10000]();
	PC = 0xfffc;
	SP = 0; //Stack at 0x0100
	A = 0;
	X = 0;
	Y = 0;
	P = 0;

	/*
	 * Bits of P:
	 * 0: Carry Flag
	 * 1: Zero Flag
	 * 2: Interrupt Disable
	 * 3: Decimal Mode (Unused in NES)
	 * 4: Break Command
	 * 5: [Unused]
	 * 6: Overflow Tag
	 * 7: Negative Flag
	 */

	rom = _rom;

	for(int i = 0; i < KB16; ++i) {
		memory[0x8000+i] = rom->prg_rom[i];
	}

	if(rom->prg_banks <= 1)
		for(int i = 0; i < KB16; ++i)
			memory[0xC000+i] = rom->prg_rom[i];
	else
		for(int i = 0; i < KB16; ++i)
			memory[0xC000+i] = rom->prg_rom[KB16+i];

	uint8_t startPClow = memory[PC];
	uint8_t startPChigh = memory[PC+1];

#if CPU_DEBUG
		printf("Constructing PC out of %02x and %02x\n", startPClow, startPChigh);
		printf("Found at %04x which should be %04x in the PRG\n", PC, rom->prg_banks <= 1 ? PC-0xC000 : PC-0x8000);
		printf("Which should be %04x in the ROM\n", rom->prg_banks <= 1 ? PC-0xC000+16 : PC-0x8000+16);
#endif

	PC = combineLowHigh(startPClow, startPChigh);

#if NESTEST
		PC = 0xc000;
#endif

#if CPU_DEBUG
		printf("PC is now %04x\n", PC);
#endif

}


void NES_CPU::pushPCtoStack() {
	uint8_t high = (PC & 0xff00) >> 8;
	uint8_t low = PC & 0x00ff;

	SP--;
	memory[0x0100 + SP] = high;
	SP--;
	memory[0x0100 + SP] = low;

#if CPU_DEBUG
		printf("Pushed PC (%04x) to Stack\n", PC);
#endif
}

void NES_CPU::retrievePCfromStack() {
	uint8_t low = memory[0x0100 + SP++];
	uint8_t high = memory[0x0100 + SP++];

#if CPU_DEBUG
		printf("PC is now %04x, retrieving from stack...\n", PC);
#endif

	PC = combineLowHigh(low, high);

#if CPU_DEBUG
		printf("Retrieved %04x from Stack\n", PC);
#endif
}

inline void NES_CPU::pushToStack(uint8_t value) { memory[--SP] = value; }
inline uint8_t NES_CPU::pullFromStack() { return memory[SP++]; }


uint8_t NES_CPU::ADC() {
	/*
	 * Add with Carry
	 * A += target + Carry
	 * Sets Carry Flag if Overflow occurs
	 * Sets Overflow Flag if Overflow occurs
	 * Sets Zero Flag if A==0
	 * Sets Negative if bit 7 of A is set
	 */

	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x69:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0x65:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0x75:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0x6d:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x7d:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	case 0x79:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0x61:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0x71:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target = getIndirectYValue();
		break;

	default:
		printf("ADC code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	bool bit7before = isBitSet(A,7);

	A += target;
	if(isSetCarryFlag()) A++;

	bool overflow = bit7before != isBitSet(A,7);

	setCarryFlag(overflow);
	setOverflow(overflow);
	setZeroFlag(A==0);
	setNegative(isBitSet(A,7));

	PC += bytes;
	return cycles;
}

uint8_t NES_CPU::AND() { //performs & on A, setting Zero and Negative when Appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x21:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0x25:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0x29:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0x2d:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x31:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target = getIndirectYValue();
		break;

	case 0x35:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0x39:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0x3d:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	default:
		printf("AND code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	A &= target;
	setZeroFlag(A == 0);
	setNegative(isBitSet(A, 7));

	PC += bytes;
	return cycles;
}

uint8_t NES_CPU::ASL() { //Shifts target one bit to the left, places bit 7 in the carry flag
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t* target = NULL;
	switch(memory[PC]) {

	case 0x06:
		bytes = 2;
		cycles = 5;
		target = &memory[memory[PC+1]];
		break;


	default:
		printf("ASL code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

#if CPU_DEBUG
		printf("Doing ASL on %02x\n", *target);
#endif

	//if(isBitSet(*target, 7)) setBit(&P, 0, 1);
	//else setBit(&P, 0, 0);
	setCarryFlag(isBitSet(*target, 7));
	(*target) = (*target) << 1;
	setNegative(isBitSet(*target, 7));

#if CPU_DEBUG
		printf("Did ASL, P is now %02x\n", P);
#endif


	PC+=bytes;
	return cycles;
}


uint8_t NES_CPU::BIT() {
	/*
	 * A & target in memory (does NOT change A)
	 * Sets Zero if the result is 0
	 * Sets Overflow to bit 6 of the memory value
	 * Sets Negative to bit 7 of the memory value
	 */
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x2c:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x24:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	default:
		printf("BIT code %02x does not exist\n", memory[PC]);
		return 0;
	}

	uint8_t result = A & target;
	setZeroFlag(result == 0);
	setOverflow(isBitSet(target, 6));
	setNegative(isBitSet(target, 7));

	PC += bytes;
	return cycles;
}


uint8_t NES_CPU::BRK() {
	/*
	 * First pushes PC+2 to stack, then P
	 * Loads Interrupt Vector from 0xfffe/f into PC
	 * sets BRK
	 */

	PC += 2;

	pushPCtoStack();

	pushToStack(P);

#if CPU_DEBUG
		printf("Doing BRK, PC before was %04x\n", PC);
#endif

	uint8_t low = memory[0xfffe];
	uint8_t high = memory[0xffff];

	PC = combineLowHigh(low, high);


#if CPU_DEBUG
		printf("PC is now %04x\n", PC);
#endif


	setBRK(1);


	return 7;
}

/*uint8_t NES_CPU::BVC() { //Branch if overflow clear

	if(!isSetOverflow()) {
		branchRelative();
		PC += 2;
		return 3; //TODO: +2 if new page
	} else {
		PC+=2;
		return 2;
	}

}*/

uint8_t NES_CPU::CMP(uint8_t* Z) {
	/*
	 * Compares A to target
	 * Sets Carry to A>=target
	 * Sets Zero to A==target
	 * Sets Negative if bit 7 of target is set
	 */
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0xc1:
		bytes = 2;
		cycles = 6;
		target =getIndirectXValue();
		break;

	case 0xc5:
	case 0xe4:
	case 0xc4:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0xc9:
	case 0xe0:
	case 0xc0:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0xcd:
	case 0xec:
	case 0xcc:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0xd1:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target =getIndirectYValue();
		break;

	case 0xd5:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0xd9:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0xdd:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	default:
		printf("CMP code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	setCarryFlag((*Z)>=target);
	setZeroFlag((*Z)==target);
	setNegative(isBitSet(target, 7));

	PC += bytes;
	return cycles;

}

uint8_t NES_CPU::DEZ(uint8_t* Z) { //Decrements Z, setting Zero and Negative when appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;

	switch(memory[PC]) {

	case 0xca:
		bytes = 1;
		cycles = 2;
		break;

	case 0xce:
		bytes = 3;
		cycles = 6;
		break;

	default:
		printf("DEZZ code %02x not implemented yet\n", memory[PC]);
		return 0;
	}


	(*Z)--;
	setZeroFlag(*Z==0);
	setNegative(isBitSet(*Z, 7));

	PC += bytes;
	return cycles;
}

uint8_t NES_CPU::EOR() { //performs bitwise XOR on A, setting Zero and Negative as appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x49:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0x45:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0x55:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0x4d:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x5d:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	case 0x59:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0x41:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0x51:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target = getIndirectYValue();
		break;

	default:
		printf("EOR code %02x not implemented yet\n", memory[PC]);
	}

	A ^= target;

	setZeroFlag(A==0);
	setNegative(isBitSet(A, 7));

	PC+=bytes;
	return cycles;

}

uint8_t NES_CPU::INZ(uint8_t* Z) { //Increments X, Y or a location in memory, setting Zero and Negative when appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;

	switch(memory[PC]) {

	case 0xe8:
		bytes = 1;
		cycles = 2;
		break;

	case 0xf6:
		bytes = 2;
		cycles = 6;
		break;

	default:
		printf("INZ code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	(*Z)++;
	setZeroFlag(*Z==0);
	setNegative(isBitSet(*Z, 7));

	PC += bytes;
	return cycles;
}

uint8_t NES_CPU::JSR() { //Jump to Subroutine, takes 3 bytes but only pushes PC+2 onto stack
	uint16_t addr = getAbsoluteAddress();
	PC += 3; //6502 reference at obelisk says this should only be increased by 2, but in the nestest log it's increased by 3
	pushPCtoStack();
	PC = addr;

	return 6;

}

uint8_t NES_CPU::ORA() { //performs bitwise OR on A, setting Zero and Negative as appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x09:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0x05:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0x15:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0x0d:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x1d:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	case 0x19:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0x01:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0x11:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target = getIndirectYValue();
		break;

	default:
		printf("ORA code %02x not implemented yet\n", memory[PC]);
	}

	A |= target;

	setZeroFlag(A==0);
	setNegative(isBitSet(A, 7));

	PC+=bytes;
	return cycles;

}

uint8_t NES_CPU::PLA() { //Pulls value from stack into A, setting Zero and Negative as appropriate
	A = pullFromStack();
	setZeroFlag(A==0);
	setNegative(isBitSet(A,7));
	PC++;
	return 4;
}

uint8_t NES_CPU::JMP() { //Jumps to target
	uint8_t cycles = 0;
	uint16_t target = 0;

	switch(memory[PC]) {

	case 0x4c:
		cycles = 3;
		target = getAbsoluteAddress();
		break;

	case 0x6c:
		cycles = 5;
		target = getIndirectValue(); //TODO: Fix to Indirect Address
		break;

	default:
		printf("JMP Code %02x does not exist\n", memory[PC]);
		return 0;
	}

	PC = target;

	return cycles;

}

uint8_t NES_CPU::LDZ(uint8_t* Z) { //loads a byte into A, X or Y, setting Zero and Negative Flags when applicable
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0xa1:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0xa5:
	case 0xa6:
	case 0xa4:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0xa9: //Immediate addressing mode
	case 0xa2:
	case 0xa0:
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0xad: //Absolute addressing mode
	case 0xae:
	case 0xac:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0xb1:
		bytes = 2;
		cycles = 5; //TODO: +1 if page crossed
		target = getIndirectYValue();
		break;

	case 0xb5:
	case 0xb4:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0xb6:
		bytes = 2;
		cycles = 4;
		target = getZeroPageYValue();
		break;

	case 0xb9:
	case 0xbe:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteYValue();
		break;

	case 0xbd:
	case 0xbc:
		bytes = 3;
		cycles = 4; //TODO: +1 if page crossed
		target = getAbsoluteXValue();
		break;

	default:
		printf("LDZ code %02x does not exist\n", memory[PC]);
		return 0;
	}

	*Z = target;
	setZeroFlag(*Z==0);
	setNegative(isBitSet(*Z, 7));

	PC += bytes;
	return cycles;

}

uint8_t NES_CPU::LSR() {
	/*
	 * Shifts the target one to the right
	 * sets Carry to old contents of bit 0
	 * sets Zero Flag if result is 0
	 * sets Negative Flag if bit 7 of result is set (should never happen)
	 */
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t* target = NULL;

	switch(memory[PC]) {

	case 0x4a:
		bytes = 1;
		cycles = 2;
		target = &A;
		break;

	default:
		printf("LSR code %02x not implemented yet\n", memory[PC]);
	}

	setCarryFlag(isBitSet(*target, 0));
	(*target) = (*target) >> 1;
	setZeroFlag(*target == 0);
	if(isBitSet(*target, 7)) {
		printf("ERROR: Bit 7 is set after LSR\n");
		setNegative(1);
	} else setNegative(0);

	PC += bytes;
	return cycles;

}

inline uint8_t NES_CPU::RTI() { P = pullFromStack(); retrievePCfromStack(); return 6; }

uint8_t NES_CPU::SBC() {
	/*
	 * A-M-(1-C)
	 * Subtracts the target and the NOT of the Carry bit from A
	 * Clears the Carry Flag if an overflow occurs
	 * Sets Zero Flag if A==0
	 * Sets Overflow Flag if overflow occurs
	 * Sets Negative Flag if bit 7 of A is set afterwards
	 */
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0xe1:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0xe5:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0xe9:
	case 0xeb: //Unofficial opcode
		bytes = 2;
		cycles = 2;
		target = getImmediateValue();
		break;

	case 0xed:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0xf1:
		bytes = 2;
		cycles = 5; //TODO:Increase cycles if page is crossed
		target = getIndirectYValue();
		break;

	case 0xf5:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0xf9:
		bytes = 3;
		cycles = 4; //TODO:Increase cycles if page crossed
		target= getAbsoluteYValue();
		break;

	case 0xfd:
		bytes = 3;
		cycles = 4; //TODO:Increase cycles if page crossed
		target= getAbsoluteXValue();
		break;

	default:
		printf("SBC code %02x not implemented yet\n", memory[PC]);
	}

	bool bit7before = isBitSet(A, 7);

	A -= target;
	if(!isSetCarryFlag()) A--;

	bool bit7after = isBitSet(A, 7);

	setZeroFlag(A==0);
	setNegative(bit7after);

	if(bit7before != bit7after) {
		setCarryFlag(0);
		setOverflow(1);
	}

	PC += bytes;
	return cycles;

}

uint8_t NES_CPU::STZ(uint8_t Z) { //Stores Z into memory
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint16_t target = 0;

	switch(memory[PC]) {

	case 0x81:
		bytes = 2;
		cycles = 6;
		target = getIndirectXValue();
		break;

	case 0x85:
	case 0x86:
	case 0x84:
		bytes = 2;
		cycles = 3;
		target = getZeroPageValue();
		break;

	case 0x8d:
	case 0x8e:
	case 0x8c:
		bytes = 3;
		cycles = 4;
		target = getAbsoluteValue();
		break;

	case 0x91:
		bytes = 2;
		cycles = 6;
		target = getIndirectYValue();
		break;

	case 0x95:
	case 0x94:
		bytes = 2;
		cycles = 4;
		target = getZeroPageXValue();
		break;

	case 0x96:
		bytes = 2;
		cycles = 4;
		target = getZeroPageYValue();
		break;

	case 0x99:
		bytes = 3;
		cycles = 5;
		target = getAbsoluteYValue();
		break;

	case 0x9d:
		bytes = 3;
		cycles = 5;
		target = getAbsoluteXValue();
		break;



	default:
		printf("STZ code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	memory[target] = Z;
	PC += bytes;
	return cycles;

}


uint8_t NES_CPU::runOp() {

#if CPU_DEBUG
	printf("Executing %02x %02x at %04x, Instruction no. %i\n", memory[PC], memory[PC+1], PC,  d_totalInstructions++);
#endif

	switch(memory[PC]) {

	case 0x00:
		return BRK();
		break;

	case 0x08:
		pushToStack(P);
		PC++;
		return 3;
		break;

	case 0x09:
		case 0x05:
		case 0x15:
		case 0x0d:
		case 0x1d:
		case 0x19:
		case 0x01:
		case 0x11:
			return ORA();

	case 0x0a:
		case 0x06:
		case 0x16:
		case 0x0e:
		case 0x1e:
			return ASL();
			break;

	case 0x10:
		return BPL();

	case 0x14: //IGN - Unofficial, reads from an address and ignores it, TODO: Possible side effects of this
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xd4:
		case 0xf4:
			PC += 2;
			return 4;
			break;

	case 0x18:
		setCarryFlag(false);
		PC++;
		return 2;

	case 0x1c: //IGN with absolute addressing, not sure about cycle count
		case 0x3c:
		case 0x5c:
		case 0x7c:
		case 0xdc:
		case 0xfc:
			PC += 3;
			return 4; //maybe 5?
			break;

	case 0x1a:
		case 0x3a:
		case 0x5a:
		case 0x7a:
		case 0xda:
		case 0xea:
		case 0xfa:
			PC+=1;
			return 2; //NOP
			break;

	case 0x20:
		return JSR();

	case 0x24:
		case 0x2c:
			return BIT();

	case 0x28:
		P = pullFromStack();
		PC++;
		return 4; //PLP, Pull Processor Status

	case 0x29:
		case 0x25:
		case 0x35:
		case 0x2d:
		case 0x3d:
		case 0x39:
		case 0x21:
		case 0x31:
			return AND();

	case 0x30:
		return BMI();

	case 0x38:
		setCarryFlag(true);
		PC++;
		return 2;

	case 0x40:
		return RTI();

	case 0x48:
		pushToStack(A);
		PC++;
		return 3; //PHA, push A

	case 0x49:
		case 0x45:
		case 0x55:
		case 0x4d:
		case 0x5d:
		case 0x59:
		case 0x41:
		case 0x51:
			return EOR();

	case 0x4a:
		case 0x46:
		case 0x56:
		case 0x4e:
		case 0x5e:
			return LSR();

	case 0x4c:
		case 0x6c:
			return JMP();

	case 0x50:
		return BVC();

	case 0x60:
		retrievePCfromStack();
		return 6; //RTS, return from subroutine

	case 0x68:
		return PLA();

	case 0x69:
		case 0x65:
		case 0x75:
		case 0x6d:
		case 0x7d:
		case 0x79:
		case 0x61:
		case 0x71:
			return ADC();

	case 0x70:
		return BVS();

	case 0x78:
		setInterruptDisable(1);
		PC+=1;
		return 2;
		break;

	case 0x84:
		case 0x94:
		case 0x8c:
			return STZ(Y);
			break;

	case 0x85:
		case 0x95:
		case 0x8d:
		case 0x9d:
		case 0x99:
		case 0x81:
		case 0x91:
			return STZ(A);
			break;

	case 0x86:
		case 0x96:
		case 0x8e:
			return STZ(X);
			break;

	case 0x90:
		return BCC();

	case 0x9a:
		SP = X;
		PC += 1;
		return 2;

	case 0xa2:
		case 0xa6:
		case 0xb6:
		case 0xae:
		case 0xbe:
			return LDZ(&X);
			break;

	case 0xa9:
		case 0xa5:
		case 0xb5:
		case 0xad:
		case 0xbd:
		case 0xb9:
		case 0xa1:
		case 0xb1:
			return LDZ(&A);
			break;

	case 0xb0:
		return BCS();

	case 0xb8:
		setOverflow(false);
		PC++;
		return 2;

	case 0xc0:
		case 0xc4:
		case 0xcc:
			return CMP(&Y);
			break;


	case 0xc9:
		case 0xc5:
		case 0xd5:
		case 0xcd:
		case 0xdd:
		case 0xd9:
		case 0xc1:
		case 0xd1:
			return CMP(&A);
			break;

	case 0xca:
		return DEZ(&X);
		break;

	case 0xce:
		return DEZ(&memory[combineLowHigh(memory[PC+1], memory[PC+2])]);
		break;

	case 0xd0:
		return BNE();

	case 0xd8:
		setDecimalMode(0);
		PC+=1;
		return 2;
		break;

	case 0xe0:
		case 0xe4:
		case 0xec:
			return CMP(&X);

	case 0xe8:
		return INZ(&X);

	case 0xe9:
		case 0xeb:
		case 0xe5:
		case 0xf5:
		case 0xed:
		case 0xfd:
		case 0xf9:
		case 0xe1:
		case 0xf1:
			return SBC();

	case 0xf0:
		return BEQ();

	case 0xf6:
		return INZ(&memory[(uint8_t) (memory[PC+1] + X)]);
		break;

	case 0xf8:
		setDecimalMode(1);
		PC++;
		return 2;
		break;




	default:
		printf("Opcode %02x not implemented yet\n", memory[PC]);
		printf("Found at %04x which should be %04x in the PRG\n", PC, rom->prg_banks <= 1 ? PC-0xC000 : PC-0x8000);
		printf("Which should be %04x in the ROM\n", rom->prg_banks <= 1 ? PC-0xC000+16 : PC-0x8000+16);
		return 0;
	}
}

uint8_t NES_CPU::branchIfFlagSet(bool flag, bool isSet) {
	bool branching = flag == isSet;
	if(branching) {
		branchRelative();
		PC += 2;
		return 3; //TODO: +2 if new page
	} else {
		PC += 2;
		return 2;
	}
}

inline uint8_t NES_CPU::BCS() {return branchIfFlagSet(isSetCarryFlag(), true); }
inline uint8_t NES_CPU::BCC() {return branchIfFlagSet(isSetCarryFlag(), false); }
inline uint8_t NES_CPU::BEQ() {return branchIfFlagSet(isSetZeroFlag(), true); }
inline uint8_t NES_CPU::BNE() {return branchIfFlagSet(isSetZeroFlag(), false); }
inline uint8_t NES_CPU::BMI() {return branchIfFlagSet(isSetNegative(), false); }
inline uint8_t NES_CPU::BPL() {return branchIfFlagSet(isSetNegative(), true); }
inline uint8_t NES_CPU::BVC() {return branchIfFlagSet(isSetOverflow(), false); }
inline uint8_t NES_CPU::BVS() {return branchIfFlagSet(isSetOverflow(), true); }


inline void NES_CPU::setCarryFlag(bool value) { setBit(&P, 0, value); }
inline void NES_CPU::setZeroFlag(bool value) { setBit(&P, 1, value); }
inline void NES_CPU::setInterruptDisable(bool value) { setBit(&P, 2, value); }
inline void NES_CPU::setDecimalMode(bool value) { setBit(&P, 3, value); }
inline void NES_CPU::setBRK(bool value) { setBit(&P, 4, value); }
inline void NES_CPU::setOverflow(bool value) { setBit(&P, 6, value); }
inline void NES_CPU::setNegative(bool value) { setBit(&P, 7, value); }

inline bool NES_CPU::isSetCarryFlag() {return isBitSet(P, 0); }
inline bool NES_CPU::isSetZeroFlag() {return isBitSet(P, 1); }
inline bool NES_CPU::isSetInterruptDisable() {return isBitSet(P, 2); }
inline bool NES_CPU::isSetDecimalMode() {return isBitSet(P, 3); }
inline bool NES_CPU::isSetBRK() {return isBitSet(P, 4); }
inline bool NES_CPU::isSetOverflow() {return isBitSet(P, 6); }
inline bool NES_CPU::isSetNegative() {return isBitSet(P, 7); }

inline uint8_t NES_CPU::getImmediateValue() {return memory[PC+1]; }
inline uint8_t NES_CPU::getZeroPageValue() {return memory[memory[PC+1]]; }
inline uint8_t NES_CPU::getZeroPageXValue() {return memory[memory[PC+1]+X]; }
inline uint8_t NES_CPU::getZeroPageYValue() {return memory[memory[PC+1]+Y]; }
inline uint8_t NES_CPU::getAbsoluteValue() {return memory[combineLowHigh(memory[PC+1], memory[PC+2])]; }
inline uint16_t NES_CPU::getAbsoluteAddress() {return combineLowHigh(memory[PC+1], memory[PC+2]); }
inline uint8_t NES_CPU::getAbsoluteXValue() {return memory[combineLowHigh(memory[PC+1], memory[PC+2])+X]; }
inline uint8_t NES_CPU::getAbsoluteYValue() {return memory[combineLowHigh(memory[PC+1], memory[PC+2])+Y]; }
inline uint8_t NES_CPU::getIndirectValue() {
	uint8_t addr = getAbsoluteValue();
	return memory[combineLowHigh(memory[addr], memory[addr+1])];
}
inline uint8_t NES_CPU::getIndirectXValue() {
	uint8_t addr = memory[PC+1] + X;
	return memory[combineLowHigh(memory[addr], memory[addr+1])];
}
inline uint8_t NES_CPU::getIndirectYValue() {
	uint8_t addr = memory[PC+1] + Y;
	return memory[combineLowHigh(memory[addr], memory[addr+1])];
}

inline void NES_CPU::branchRelative() {
	uint8_t offset = memory[PC+1];
	if(isBitSet(offset, 7)) {
		offset = offset & 0b01111111;
		PC -= offset;
	} else PC += offset;
}

void NES_CPU::d_printMemFromPC() {
	printf("Dumping the first KB of Memory located at PC: \n");
	for(int i = 0; i < 1024; ++i) {
		for(int j = 0; j++ < 16; ++i) {
			printf("%02x ", memory[PC + i]);
		}
		printf("\n");
		i--;
	}
}
