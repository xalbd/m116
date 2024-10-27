#include "CPU.h"

#include <iostream>
using namespace std;

int main(const int argc, char *argv[]) {
	if (argc < 2) {
		cout << "No file name entered. Exiting..." << endl;
		return -1;
	}

	CPU cpu;
	if (!cpu.loadIMemory(argv[1])) return 0;
	cpu.run();
	cpu.output();
	return 0;
}
