#include "ir.hpp"

#include <fstream>
#include <cstdio>

using namespace ircpp;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <input.ir> <output.ir>\n", argv[0]);
        return 1;
    }

    try {
        IRReader reader;
        IRProgram program = reader.parseIRFile(argv[1]);

        std::ofstream ofs(argv[2]);
        IRPrinter filePrinter(ofs);
        filePrinter.printProgram(program);

        IRPrinter stdoutPrinter(std::cout);
        return 0;
    } catch (const IRException& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 2;
    }
}


