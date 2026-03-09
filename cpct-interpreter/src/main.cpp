#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

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
