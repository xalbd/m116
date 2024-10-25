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
	memReadData = 0;
}

void CPU::printRegs() {
	cerr << hex << instr << endl;
	for (int i = 0; i < 32; i++) {
		cerr << i << ": " << reg[i] << endl;
	}
	for (int i = 0; i < 5; i++) {
		cerr << (int) dmemory[i] << endl;
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
		case LOAD_TYPE:
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
		case LOAD_TYPE:
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
				memRead = true;
				memToReg = true;
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
		case S_TYPE:
			regWrite = false;
			aluSrc = true;
			branch = false;
			memRead = false;
			memWrite = true;
			memToReg = false;
			aluOp = 0b00;
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
			cerr << "adding " << reg[rs1] << " " << secondAlu << endl;
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

void CPU::memory() {
	if (!memWrite && !memRead) return;

	if (memWrite) {
		cerr << "writing" << reg[rs2] << " to " << aluResult << endl;
		if (funct3 == 0) {
			dmemory[aluResult] = reg[rs2];
			cerr << "written: " << hex << (int) dmemory[aluResult] << endl;
		} else {
			cerr << dec << reg[rs2] << endl;
			dmemory[aluResult] = reg[rs2];
			dmemory[aluResult + 1] = reg[rs2] >> 8;
			dmemory[aluResult + 2] = reg[rs2] >> 16;
			dmemory[aluResult + 3] = reg[rs2] >> 24;

			cerr << "written: " << hex << (int) dmemory[aluResult] << endl;
			cerr << "written: " << hex << (int) dmemory[aluResult + 1] << endl;
			cerr << "written: " << hex << (int) dmemory[aluResult + 2] << endl;
			cerr << "written: " << hex << (int) dmemory[aluResult + 3] << endl;
		}
	} else {
		if (funct3 == 0) {
			memReadData = dmemory[aluResult];
		} else {
			cerr << "pulling from: " << aluResult << endl;
			memReadData = ((int) dmemory[aluResult + 3] << 24) +
			              ((int) dmemory[aluResult + 2] << 16) +
			              ((int) dmemory[aluResult + 1] << 8) +
			              (int) dmemory[aluResult];
		}
	}
}


void CPU::writeback() {
	cerr << "in writeback" << endl;
	cerr << regWrite << endl;
	if (regWrite) {
		cerr << "storing to " << rd << endl;
		reg[rd] = memToReg ? memReadData : aluResult;
	}
}


unsigned int CPU::readPC() const {
	return PC;
}

void CPU::incPC() {
	PC += 4;
}
