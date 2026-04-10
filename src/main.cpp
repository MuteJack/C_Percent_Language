// cpct — unified C% entry point.
// Modes: REPL (JIT), translate, compile, run.
#include "core/version.h"
#include "jit/repl.h"
#include "compile/compiler.h"
#include <iostream>
#include <string>
#include <csignal>

static void printUsage() {
    std::cout << "Usage: cpct [options] [script.cpc]" << std::endl;
    std::cout << "  cpct                                REPL mode (JIT)" << std::endl;
    std::cout << "  cpct script.cpc                     JIT execute file" << std::endl;
    std::cout << "  cpct --jit [script.cpc]              JIT mode (explicit)" << std::endl;
    std::cout << "  cpct --translate script.cpc [-o dir]  Translate to C++ (default: ./translated/)" << std::endl;
    std::cout << "  cpct --compile script.cpc [-o dir]    Compile to executable (default: ./_build/)" << std::endl;
    std::cout << "  cpct --run script.cpc                Compile and run" << std::endl;
    std::cout << "  cpct --version                       Show version" << std::endl;
    std::cout << "  cpct --help                          Show this help" << std::endl;
}

static std::string getExeBaseDir(const char* argv0) {
    std::string exePath = argv0;
    auto ls = exePath.find_last_of("/\\");
    return (ls != std::string::npos) ? exePath.substr(0, ls + 1) : "";
}

// Parse -o option from argv, returns the value or defaultVal
static std::string parseOutputDir(int argc, char* argv[], int startIdx, const std::string& defaultVal) {
    for (int i = startIdx; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            return argv[i + 1];
        }
    }
    return defaultVal;
}

// Find the input file from argv (first arg that doesn't start with -)
static std::string findInputFile(int argc, char* argv[], int startIdx) {
    for (int i = startIdx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-') return arg;
        if ((arg == "-o" || arg == "--output") && i + 1 < argc) i++; // skip -o value
    }
    return "";
}

int main(int argc, char* argv[]) {
    std::string exeBaseDir = getExeBaseDir(argv[0]);

    // No args → REPL
    if (argc < 2) {
        auto repl = ClingRepl::autoDetect(argv[0]);
        if (!repl.isAvailable()) return 1;
        repl.run();
        return 0;
    }

    std::string first = argv[1];

    // --help
    if (first == "--help" || first == "-h") {
        printUsage();
        return 0;
    }

    // --version
    if (first == "--version" || first == "-v") {
        std::cout << "cpct v" CPCT_VERSION;
        if (CPCT_NIGHTLY) std::cout << "-nightly-" CPCT_BUILD_TIME "-" CPCT_GIT_HASH;
        std::cout << std::endl;
        return 0;
    }

    // --jit [script.cpc]
    if (first == "--jit") {
        auto repl = ClingRepl::autoDetect(argv[0]);
        if (!repl.isAvailable()) return 1;
        if (argc >= 3) {
            repl.runFile(argv[2]);
        } else {
            repl.run();
        }
        return 0;
    }

    // --translate script.cpc [-o dir]
    if (first == "--translate") {
        std::string input = findInputFile(argc, argv, 2);
        if (input.empty()) {
            std::cerr << "Error: no input file specified" << std::endl;
            return 1;
        }
        std::string outputDir = parseOutputDir(argc, argv, 2, "./translated/");
        return cpctc::translateFile(input, outputDir);
    }

    // --compile script.cpc [-o dir]
    if (first == "--compile") {
        std::string input = findInputFile(argc, argv, 2);
        if (input.empty()) {
            std::cerr << "Error: no input file specified" << std::endl;
            return 1;
        }
        std::string outputDir = parseOutputDir(argc, argv, 2, "./_build/");
        return cpctc::compileFile(input, outputDir, exeBaseDir);
    }

    // --run script.cpc
    if (first == "--run") {
        if (argc < 3) {
            std::cerr << "Error: no input file specified" << std::endl;
            return 1;
        }
        return cpctc::compileAndRun(argv[2], exeBaseDir);
    }

    // Default: treat as .cpc file → JIT execute
    auto repl = ClingRepl::autoDetect(argv[0]);
    if (!repl.isAvailable()) return 1;
    repl.runFile(first);
    return 0;
}
