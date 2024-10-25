#include "CPU.h"

CPU::CPU() : dmemory(), imemory(), reg() {
	PC = 0;
	instr = 0;
	opcode = 0;
	rs1 = 0;
	rs2 = 0;
	rd = 0;
	funct3 = 0;
	funct7 = 0;
	imm = 0;
	regWrite = false;
	aluSrc = false;
	branch = false;
	memRead = false;
	memWrite = false;
	memToReg = false;
	aluOp = 0;
	aluControl = 0;
	aluResult = 0;
	aluZero = false;
}

void CPU::printRegs() {
	for (int i = 0; i < 32; i++) {
		cerr << i << ": " << reg[i] << endl;
	}
	cerr << endl;
}

void CPU::output() const {
	cout << "(" << reg[10] << "," << reg[11] << ")" << endl;
}


void CPU::setIMemory(const unsigned int addr, const unsigned char value) {
	this->imemory[addr] = value;
}

bool CPU::fetchInstruction() {
	instr = imemory[PC + 3] << 24 |
	        imemory[PC + 2] << 16 |
	        imemory[PC + 1] << 8 |
	        imemory[PC];

	return instr;
}

void CPU::decodeInstruction() {
	opcode = instr & (1 << 7) - 1;
	rd = instr >> 7 & (1 << 5) - 1;
	funct3 = instr >> 12 & (1 << 3) - 1;
	rs1 = instr >> 15 & (1 << 5) - 1;
	rs2 = instr >> 20 & (1 << 5) - 1;
	funct7 = instr >> 25 & (1 << 7) - 1;
	generateImm();
	setControlSignals();
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

void CPU::setControlSignals() {
	switch (opcode) {
		case R_TYPE:
			regWrite = true;
			aluSrc = false;
			branch = false;
			memRead = false;
			memWrite = false;
			memToReg = false;
			if (funct3 == 0x0) {
				aluOp = 0b00;
			} else {
				aluOp = 0b10;
			}
			break;
		case I_TYPE:
			regWrite = true;
			aluSrc = true;
			branch = false;
			memRead = false;
			memWrite = false;
			memToReg = false;
			if (funct3 == 6 || funct3 == 5) {
				aluOp = 0b10;
			} else {
				aluOp = 0b00;
			}
			break;
		case U_TYPE:
			regWrite = true;
			aluSrc = true;
			branch = false;
			memRead = false;
			memWrite = false;
			memToReg = false;
			break;
		default:
			regWrite = false;
			aluSrc = false;
			branch = false;
			memRead = false;
			memWrite = false;
			memToReg = false;
			break;
	}
}

void CPU::execute() {
	setALUControlSignal();
	runALU();
}

void CPU::setALUControlSignal() {
	switch (aluOp) {
		case 0b00:
			aluControl = 0b0010;
			break;
		case 0b01:
			aluControl = 0b0110;
			break;
		case 0b10:
		default:
			switch (funct3) {
				case 4:
					aluControl = 0b1001;
					break;
				case 6:
					aluControl = 0b1000;
					break;
				case 5:
					aluControl = 0b0111;
					break;
				default:
					break;
			}
	}
}

void CPU::runALU() {
	const int secondAlu = aluSrc ? imm : reg[rs2];
	switch (aluControl) {
		case 0b0010:
			aluResult = reg[rs1] + secondAlu;
			break;
		case 0b0110:
			aluResult = reg[rs1] - secondAlu;
			break;
		case 0b1000:
			aluResult = reg[rs1] | secondAlu;
			break;
		case 0b1001:
			aluResult = reg[rs1] ^ secondAlu;
			break;
		case 0b0111:
			aluResult = reg[rs1] >> secondAlu;
		default:
			break;
	}

	aluZero = aluResult == 0;
}


void CPU::writeback() {
	if (regWrite && !memToReg) {
		reg[rd] = aluResult;
	}
}


unsigned int CPU::readPC() const {
	return PC;
}

void CPU::incPC() {
	PC += 4;
}
