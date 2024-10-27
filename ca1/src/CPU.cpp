#include "CPU.h"
#include <fstream>

CPU::CPU() : imemory(), reg() {
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
	useRS1 = false;
	forceJump = false;
	byteOnly = false;
	aluOp = 0;
	aluControl = 0;
	aluResult = 0;
	aluZero = false;
	memReadData = 0;
}

bool CPU::loadIMemory(const char* file) {
	ifstream infile(file);
	if (!(infile.is_open() && infile.good())) {
		cout << "Error opening file. Exiting..." << endl;
		return false;
	}

	string line;
	int i = 0;
	while (infile) {
		infile >> line;
		setIMemory(i, stoi(line, nullptr, 16));
		i++;
	}

	return true;
}


void CPU::run() {
	while (true) {
		fetchInstruction();
		if (instr == 0) {
			break;
		}

		decodeInstruction();
		execute();
		memory();
		writeback();
		incPC();

		if (PC >= MEMORY_LIMIT) {
			break;
		}
	}
}


void CPU::output() const {
	cout << "(" << reg[10] << "," << reg[11] << ")" << endl;
}


void CPU::setIMemory(const unsigned int addr, const unsigned int value) {
	this->imemory[addr] = static_cast<unsigned char>(value);
}

bool CPU::fetchInstruction() {
	instr = (static_cast<int>(imemory[PC + 3]) << 24) +
	        (static_cast<int>(imemory[PC + 2]) << 16) +
	        (static_cast<int>(imemory[PC + 1]) << 8) +
	        static_cast<int>(imemory[PC]);

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
	bitset<32> temp(0);
	bitset<32> orig(instr);

	switch (opcode) {
		case I_TYPE:
		case LOAD_TYPE:
			for (int i = 0; i <= 11; i++) {
				temp[i] = orig[20 + i];
			}
			for (int i = 12; i <= 31; i++) {
				temp[i] = temp[11];
			}
			break;
		case S_TYPE:
			for (int i = 0; i <= 4; i++) {
				temp[i] = orig[7 + i];
			}
			for (int i = 5; i <= 11; i++) {
				temp[i] = orig[20 + i];
			}
			for (int i = 12; i <= 31; i++) {
				temp[i] = temp[11];
			}
			break;
		case B_TYPE:
			for (int i = 1; i <= 4; i++) {
				temp[i] = orig[7 + i];
			}
			for (int i = 5; i <= 10; i++) {
				temp[i] = orig[20 + i];
			}
			temp[11] = orig[7];
			temp[12] = orig[31];
			for (int i = 13; i <= 31; i++) {
				temp[i] = temp[12];
			}
			break;
		case U_TYPE:
			for (int i = 12; i <= 31; i++) {
				temp[i] = orig[i];
			}
			break;
		case J_TYPE:
			for (int i = 1; i <= 10; i++) {
				temp[i] = orig[20 + i];
			}
			for (int i = 12; i <= 19; i++) {
				temp[i] = orig[i];
			}
			temp[20] = orig[31];
			temp[11] = orig[20];
			for (int i = 21; i <= 31; i++) {
				temp[i] = temp[20];
			}
			break;
		default:
			break;
	}

	imm = static_cast<int>(temp.to_ulong());
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
			useRS1 = true;
			forceJump = false;
			byteOnly = false;
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
			useRS1 = true;
			forceJump = false;
			byteOnly = false;
			if (funct3 == 6 || funct3 == 5) {
				aluOp = 0b10;
			} else {
				aluOp = 0b00;
				memRead = true;
				memToReg = true;
				byteOnly = funct3 == 0;
			}
			break;
		case U_TYPE:
			regWrite = true;
			aluSrc = true;
			branch = false;
			memRead = false;
			memWrite = false;
			memToReg = false;
			useRS1 = false;
			forceJump = false;
			byteOnly = false;
			aluOp = 0b00;
			break;
		case S_TYPE:
			regWrite = false;
			aluSrc = true;
			branch = false;
			memRead = false;
			memWrite = true;
			memToReg = false;
			useRS1 = true;
			forceJump = false;
			byteOnly = funct3 == 0;
			aluOp = 0b00;
			break;
		case B_TYPE:
			regWrite = false;
			aluSrc = false;
			branch = true;
			memRead = false;
			memWrite = false;
			memToReg = false;
			useRS1 = true;
			forceJump = false;
			byteOnly = false;
			aluOp = 0b01;
			break;
		case J_TYPE:
			regWrite = true;
			aluSrc = false;
			branch = true;
			memRead = false;
			memWrite = false;
			memToReg = false;
			useRS1 = true;
			forceJump = true;
			byteOnly = false;
			aluOp = 0b00;
			break;
		default:
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
	const int firstAlu = useRS1 ? reg[rs1] : 0;
	const int secondAlu = aluSrc ? imm : reg[rs2];
	switch (aluControl) {
		case 0b0010:
			aluResult = firstAlu + secondAlu;
			break;
		case 0b0110:
			aluResult = firstAlu - secondAlu;
			break;
		case 0b1000:
			aluResult = firstAlu | secondAlu;
			break;
		case 0b1001:
			aluResult = firstAlu ^ secondAlu;
			break;
		case 0b0111:
			aluResult = firstAlu >> (secondAlu & 0b11111);
		default:
			break;
	}

	aluZero = aluResult == 0;
}

void CPU::memory() {
	if (!memWrite && !memRead) return;

	if (memWrite) {
		bitset<32> write(reg[rs2]);
		if (byteOnly) {
			for (int i = 0; i < 8; i++) {
				dmemory[aluResult][i] = write[i];
			}
		} else {
			for (int byte = 0; byte <= 3; byte++) {
				for (int i = 0; i < 8; i++) {
					dmemory[aluResult + byte][i] = write[byte * 8 + i];
				}
			}
		}
	} else {
		bitset<32> read(0);
		if (byteOnly) {
			for (int i = 0; i < 8; i++) {
				read[i] = dmemory[aluResult][i];
			}
			for (int i = 8; i <= 31; i++) {
				read[i] = read[7];
			}
		} else {
			for (int byte = 0; byte <= 3; byte++) {
				for (int i = 0; i < 8; i++) {
					read[byte * 8 + i] = dmemory[aluResult + byte][i];
				}
			}
		}

		memReadData = static_cast<int>(read.to_ulong());
	}
}


void CPU::writeback() {
	if (regWrite && rd != 0) {
		reg[rd] = memToReg ? memReadData : (forceJump ? PC + 4 : aluResult);
	}
}

void CPU::incPC() {
	PC += (branch && aluZero) || forceJump ? imm : 4;
}
