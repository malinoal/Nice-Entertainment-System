#include "header.h"

#include "NES_CPU.h"
#include "helper.h"

#define CPU_DEBUG 1

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

	PC = combineLowHigh(startPClow, startPChigh);

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



uint8_t NES_CPU::AND() { //performs & on A, setting Zero and Negative when Appropriate
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0x29:
		bytes = 2;
		cycles = 2;
		target = memory[PC+1];
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

uint8_t NES_CPU::BENQ(bool value) { //branches if the Zero Flag is Set to value, interpreting the offset as a signed bytes
	uint8_t cycles = 2;

	PC += 2;

	if(isSetZeroFlag()==value) {
		cycles++; //TODO: increment cycles once more if on a new page
		uint8_t offset = memory[PC-1];
		if(isBitSet(offset, 7)) {
			offset = offset & 0b01111111;
			PC -= offset;
		} else PC += offset;
	}

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
		target = memory[combineLowHigh(memory[PC+1], memory[PC+2])];
		break;

	case 0x24:
		bytes = 2;
		cycles = 3;
		target = memory[memory[PC+1]];
		break;

	default:
		printf("BIT code %02x not implemented yet\n", memory[PC]);
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

uint8_t NES_CPU::BVC() { //Branch if overflow clear

	if(!isSetOverflow()) {
		uint8_t offset = memory[PC+1];
		if(isBitSet(offset, 7)) {
			offset = offset & 0b01111111;
			PC -= offset;
		} else PC += offset;
		PC += 2;
		return 3; //TODO: +2 if new page
	} else {
		PC+=2;
		return 2;
	}

}

uint8_t NES_CPU::CMP() {
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

	case 0xc9:
		bytes = 2;
		cycles = 2;
		target = memory[PC+1];
		break;

	default:
		printf("CMP code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	setCarryFlag(A>=target);
	setZeroFlag(A==target);
	setNegative(isBitSet(target, 7));

	PC += bytes;
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

uint8_t NES_CPU::LDZ(uint8_t* Z) { //loads a byte into A, X or Y, setting Zero and Negative Flags when applicable
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint8_t target = 0;

	switch(memory[PC]) {

	case 0xa9: //Immediate addressing mode
	case 0xa2:
		bytes = 2;
		cycles = 2;
		target = memory[PC+1];
		break;

	case 0xad: //Absolute addressing mode
		bytes = 3;
		cycles = 4;
		target = memory[combineLowHigh(memory[PC+1], memory[PC+2])];
		break;

	default:
		printf("LDZ code %02x not implemented yet\n", memory[PC]);
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

	case 0xe9:
	case 0xeb: //Unofficial opcode
		bytes = 2;
		cycles = 2;
		target = memory[PC+1];
		break;

	case 0xed:
		bytes = 3;
		cycles = 4;
		target = memory[combineLowHigh(memory[PC+1], memory[PC+2])];
		break;

	case 0xf1:
		bytes = 2;
		cycles = 5; //TODO:Increase cycles if page is crossed
		{uint16_t addr = memory[PC+1] + Y;
		target = memory[combineLowHigh(memory[addr], memory[addr+1])];}
		break;

	case 0xf9:
		bytes = 3;
		cycles = 4; //TODO:Increase cycles if page crossed
		target= memory[combineLowHigh(memory[PC+1], memory[PC+2]) + Y];
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

uint8_t NES_CPU::STA() { //Stores accumulator into memory
	uint8_t bytes = 0;
	uint8_t cycles = 0;
	uint16_t target = 0;

	switch(memory[PC]) {

	case 0x8d:
		bytes = 3;
		cycles = 4;
		target = combineLowHigh(memory[PC+1], memory[PC+2]);
		break;



	default:
		printf("STA code %02x not implemented yet\n", memory[PC]);
		return 0;
	}

	memory[target] = A;
	PC += bytes;
	return cycles;

}


uint8_t NES_CPU::runOp() {

#if CPU_DEBUG
	printf("Executing %02x %02x, Instruction no. %i\n", memory[PC], memory[PC+1], d_totalInstructions++);
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

	case 0x0a:
		case 0x06:
		case 0x16:
		case 0x0e:
		case 0x1e:
			return ASL();
			break;

	case 0x14: //IGN - Unofficial, reads from an address and ignores it, TODO: Possible side effects of this
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xd4:
		case 0xf4:
			PC += 2;
			return 4;
			break;

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

	case 0x24:
		case 0x2c:
			return BIT();

	case 0x29:
		case 0x25:
		case 0x35:
		case 0x2d:
		case 0x3d:
		case 0x39:
		case 0x21:
		case 0x31:
			return AND();

	case 0x40:
		return RTI();

	case 0x4a:
		case 0x46:
		case 0x56:
		case 0x4e:
		case 0x5e:
			return LSR();

	case 0x50:
		return BVC();
		break;

	case 0x78:
		setInterruptDisable(1);
		PC+=1;
		return 2;
		break;

	case 0x85:
		case 0x95:
		case 0x8d:
		case 0x9d:
		case 0x99:
		case 0x81:
		case 0x91:
			return STA();
			break;

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


	case 0xc9:
		case 0xc5:
		case 0xd5:
		case 0xcd:
		case 0xdd:
		case 0xd9:
		case 0xc1:
		case 0xd1:
			return CMP();
			break;

	case 0xd0:
		return BENQ(false);

	case 0xd8:
		setDecimalMode(0);
		PC+=1;
		return 2;
		break;

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
		return BENQ(true);
		break;

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
		printf("Which should be %04x in the ROM\n", rom->prg_banks <= 1 ? PC-0xC000+10 : PC-0x8000+10);
		return 0;
	}
}



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
