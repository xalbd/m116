#include "CPU.h"

#include <iostream>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
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
		const unsigned int grabbedValue = stoul(line, nullptr, 16);

		if (i >= MEMORY_LIMIT) {
			cout << "Instruction memory too long. Exiting..." << endl;
		}
		else if (grabbedValue > numeric_limits<unsigned char>::max()) {
			cout << "Error parsing instruction. Exiting..." << endl;
			return 0;
		}

		cpu.setIMemory(i, grabbedValue);
		i++;
	}

	// Main loop
	bool done = true;
	while (done == true) {
		cpu.fetchInstruction();
		cpu.decodeInstruction();

		cpu.incPC();
		if (cpu.readPC() >= MEMORY_LIMIT / 4) {
			break;
		}
	}

	int a0 = 0;
	int a1 = 0;
	cout << "(" << a0 << "," << a1 << ")" << endl;

	return 0;
}
