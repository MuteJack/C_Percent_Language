// C% interpreter entry point
// Supports both file execution mode and interactive REPL mode.
#include "lexer.h"
#include "parser.h"
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

// Runs the source string through the lexer → parser → interpreter pipeline.
static void runSource(const std::string& source, Interpreter& interp) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto program = parser.parse();

        interp.run(program);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Interactive REPL — tracks brace depth to accept multi-line input until blocks are closed.
static void repl() {
    std::cout << "C% Interpreter v0.1.0" << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;
    std::cout << std::endl;

    Interpreter interp;
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

// Runs in REPL mode with no arguments, or file execution mode when a file path is given.
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // REPL mode
        repl();
    } else {
        std::string filename = argv[1];
        std::string source = readFile(filename);

        Interpreter interp;
        runSource(source, interp);
    }

    return 0;
}
