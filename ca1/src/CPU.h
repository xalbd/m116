#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <bitset>
using namespace std;

const unsigned int MEMORY_LIMIT = 4096;
const unsigned int R_TYPE = 0b0110011;
const unsigned int I_TYPE = 0b0010011;
const unsigned int LOAD_TYPE = 0b0000011;
const unsigned int S_TYPE = 0b0100011;
const unsigned int B_TYPE = 0b1100011;
const unsigned int J_TYPE = 0b1101111;
const unsigned int U_TYPE = 0b0110111;


class CPU {
public:
	CPU();

	bool loadIMemory(const char* file);

	void run();

	void output() const;

private:
	void setIMemory(unsigned int addr, unsigned int value);

	bool fetchInstruction();

	void decodeInstruction();

	void generateImm();

	void setControlSignals();

	void execute();

	void setALUControlSignal();

	void runALU();

	void memory();

	void writeback();

	void incPC();


	bitset<8> dmemory[MEMORY_LIMIT];
	unsigned char imemory[MEMORY_LIMIT];
	int PC;
	int reg[32];
	int instr;

	// instruction segments
	int opcode;
	int rs1;
	int rs2;
	int rd;
	int funct3;
	int funct7;
	int imm;


	// control signals
	bool regWrite;
	bool aluSrc;
	bool branch;
	bool memRead;
	bool memWrite;
	bool memToReg;
	bool useRS1;
	bool forceJump;
	bool byteOnly;
	int aluOp;

	// ALU
	int aluControl;
	int aluResult;
	bool aluZero;


	// memory
	int memReadData;
};


#endif // CPU_H
