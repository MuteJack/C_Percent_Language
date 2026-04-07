// C% interpreter entry point
// Supports both file execution mode and interactive REPL mode.
#include "../../cpct-core/src/preprocessor.h"
#include "../../cpct-core/src/lexer.h"
#include "../../cpct-core/src/parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Reads a .cpc source file and returns it as a string.
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

// Runs the source string through the preprocessor → lexer → parser → interpreter pipeline.
// Returns exit code from main() or 0 for script mode. Returns 1 on error.
static int runSource(const std::string& source, Interpreter& interp) {
    try {
        Preprocessor pp;
        std::string processed = pp.process(source);

        Lexer lexer(processed);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto program = parser.parse();

        return interp.run(program);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

// Interactive REPL — tracks brace depth to accept multi-line input until blocks are closed.
static void repl(const PlatformConfig& platform) {
    std::cout << "C% Interpreter v0.1.0 [platform: " << platform.name << "]" << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;
    std::cout << std::endl;

    Interpreter interp;
    interp.setPlatform(platform);
    std::string line;

    while (true) {
        std::cout << "c%> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        // Multi-line input: count braces
        std::string source = line;
        int braces = 0;
        for (char c : source) {
            if (c == '{') braces++;
            if (c == '}') braces--;
        }
        while (braces > 0) {
            std::cout << "...  ";
            if (!std::getline(std::cin, line)) break;
            source += "\n" + line;
            for (char c : line) {
                if (c == '{') braces++;
                if (c == '}') braces--;
            }
        }

        runSource(source, interp);
    }

    std::cout << "Bye!" << std::endl;
}

static void printUsage() {
    std::cerr << "Usage: cpct [--platform=<name>] [file.cpc]" << std::endl;
    std::cerr << "Platforms: default, x86, x64-linux, x64-win, avr, arm32, arm64, esp32" << std::endl;
}

// Runs in REPL mode with no arguments, or file execution mode when a file path is given.
int main(int argc, char* argv[]) {
    PlatformConfig platform;
    std::string filename;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--platform=", 0) == 0) {
            std::string name = arg.substr(11);
            platform = getPlatformPreset(name);
            if (platform.name == "default" && name != "default") {
                std::cerr << "Warning: Unknown platform '" << name << "', using default" << std::endl;
            }
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            printUsage();
            return 1;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        // REPL mode
        repl(platform);
    } else {
        std::string source = readFile(filename);

        Interpreter interp;
        interp.setPlatform(platform);
        return runSource(source, interp);
    }

    return 0;
}
