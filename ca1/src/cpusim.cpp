#include "CPU.h"

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

int main(const int argc, char *argv[]) {
	// Open file containing instructions
	if (argc < 2) {
		cout << "No file name entered. Exiting..." << endl;
		return -1;
	}

	ifstream infile(argv[1]);
	if (!(infile.is_open() && infile.good())) {
		cout << "Error opening file. Exiting..." << endl;
		return 0;
	}


	// Initialize CPU and parse instructions
	CPU cpu;

	string line;
	int i = 0;
	while (infile) {
		infile >> line;
		cpu.setIMemory(i, stoi(line, nullptr, 16));
		i++;
	}

	// Main loop
	while (true) {
		if (!cpu.fetchInstruction()) {
			break;
		}
		cpu.decodeInstruction();
		cpu.execute();
		cpu.memory();
		cpu.writeback();
		cpu.printRegs();

		cpu.incPC();
		if (cpu.readPC() >= MEMORY_LIMIT / 4) {
			break;
		}
	}

	cpu.output();
	return 0;
}
