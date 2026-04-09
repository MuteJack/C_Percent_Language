// C% Translator CLI — translates .cpc files to C++ source.
// Usage:
//   cpct-translate input.cpc              → stdout
//   cpct-translate input.cpc output.cpp   → write .cpp file
#include "../core/version.h"
#include "translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << path << "'" << std::endl;
        exit(1);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write to '" << path << "'" << std::endl;
        exit(1);
    }
    file << content;
}

static void printUsage() {
    std::cerr << "Usage: cpct-translate <input.cpc> [output.cpp]" << std::endl;
    std::cerr << "  Translates a C% source file to C++20 source." << std::endl;
    std::cerr << "  If output is omitted, prints to stdout." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        std::cout << "cpct-translate v" CPCT_VERSION << std::endl;
        return 0;
    }
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printUsage();
        return argc < 2 ? 1 : 0;
    }

    std::string inputFile = argv[1];
    std::string source = readFile(inputFile);

    try {
        Translator translator;
        std::string cppCode = translator.translate(source);

        if (argc >= 3) {
            std::string outputFile = argv[2];
            writeFile(outputFile, cppCode);
            std::cerr << "Translated: " << inputFile << " -> " << outputFile << std::endl;
        } else {
            std::cout << cppCode;
        }
    } catch (const std::exception& e) {
        std::cerr << "Translation error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
