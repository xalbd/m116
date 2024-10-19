#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace std;

constexpr unsigned int MEMORY_LIMIT = 4096;
constexpr unsigned int R_TYPE = 0b0110011;
constexpr unsigned int I_TYPE = 0b0010011;
constexpr unsigned int S_TYPE = 0b0100011;
constexpr unsigned int B_TYPE = 0b1100011;
constexpr unsigned int J_TYPE = 0b1101111;
constexpr unsigned int U_TYPE = 0b0110111;

class CPU {
public:
	void setIMemory(unsigned int addr, unsigned char value);

	unsigned int readPC() const;

	void incPC();

	void fetchInstruction();

	void decodeInstruction();

private:
	void generateImm();

	unsigned char dmemory[MEMORY_LIMIT] = {};
	unsigned char imemory[MEMORY_LIMIT] = {};
	unsigned int PC = 0;
	int reg[32] = {};
	int instr = 0;

	int opcode = 0;
	int rs1 = 0;
	int rs2 = 0;
	int rd = 0;
	int funct3 = 0;
	int funct7 = 0;
	int imm = 0;
};


#endif // CPU_H
