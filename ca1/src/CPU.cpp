#include "CPU.h"

void CPU::setIMemory(const unsigned int addr, const unsigned char value) {
	this->imemory[addr] = value;
}

void CPU::fetchInstruction() {
	instr = imemory[PC + 3] << 24 |
	        imemory[PC + 2] << 16 |
	        imemory[PC + 1] << 8 |
	        imemory[PC];
}

void CPU::decodeInstruction() {
	opcode = instr & (1 << 7) - 1;
	rd = instr >> 7 & (1 << 5) - 1;
	funct3 = instr >> 12 & (1 << 3) - 1;
	rs1 = instr >> 15 & (1 << 5) - 1;
	rs2 = instr >> 20 & (1 << 5) - 1;
	funct7 = instr >> 25 & (1 << 7) - 1;
	generateImm();


	cout << opcode << " " << rd << " " << rs1 << " " << rs2 << " " << imm << endl;
}

void CPU::generateImm() {
	switch (opcode) {
		case I_TYPE:
			imm = funct7 << 5 | rs2;
			break;
		case S_TYPE:
			imm = funct7 << 5 | rd;
			break;
		case B_TYPE:
			imm = (instr >> 31) & 1 << 12 |
				   (instr >> 8 & 0b1111) << 1 |
				   (instr >> 25) & 0b111111 << 5 |
				   (instr >> 7 & 1) << 11;
			break;
		case U_TYPE:
			imm = instr >> 12 << 12;
			break;
		case J_TYPE:
			imm = (instr >> 31) & 1 << 20 |
			       (instr >> 12 & 0b11111111) << 12 |
			       (instr >> 20) & 1 << 11 |
			       (instr >> 21 & 0b1111111111) << 1;
			break;
		default:
			imm = 0;
			break;
	}
}


unsigned int CPU::readPC() const {
	return PC;
}

void CPU::incPC() {
	PC += 4;
}
