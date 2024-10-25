#ifndef CPU_H
#define CPU_H

#include <iostream>
using namespace std;

const unsigned int MEMORY_LIMIT = 4096;
const unsigned int R_TYPE = 0b0110011;
const unsigned int I_TYPE = 0b0010011;
const unsigned int S_TYPE = 0b0100011;
const unsigned int B_TYPE = 0b1100011;
const unsigned int J_TYPE = 0b1101111;
const unsigned int U_TYPE = 0b0110111;

class CPU {
public:
	CPU();

	void setIMemory(unsigned int addr, unsigned char value);

	unsigned int readPC() const;

	void incPC();

	bool fetchInstruction();

	void decodeInstruction();

	void execute();

	void writeback();

	void printRegs();

	void output() const;

private:
	void generateImm();
	void setControlSignals();
	void setALUControlSignal();
	void runALU();

	unsigned char dmemory[MEMORY_LIMIT];
	unsigned char imemory[MEMORY_LIMIT];
	unsigned int PC;
	int reg[32];
	int instr;

	int opcode;
	int rs1;
	int rs2;
	int rd;
	int funct3;
	int funct7;
	int imm;

	bool regWrite;
	bool aluSrc;
	bool branch;
	bool memRead;
	bool memWrite;
	bool memToReg;
	int aluOp;

	int aluControl;
	int aluResult;
	bool aluZero;
};


#endif // CPU_H
