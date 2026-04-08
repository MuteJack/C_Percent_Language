// cling_repl.cpp
// C% Cling JIT REPL — translates C% to C++, sends to cling process via pipe.
#include "../../cpct-transpiler/src/translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

class ClingRepl {
public:
    ClingRepl(const std::string& clingPath, const std::string& resourceDir)
        : clingPath_(clingPath), resourceDir_(resourceDir) {}

    void run() {
        // Build cling command
        std::string cmd = "\"" + clingPath_ + "\""
                          " --nologo"
                          " -resource-dir \"" + resourceDir_ + "\""
                          " -std=c++20";

        // Open pipe to cling (write mode — cling's stdout goes to our stdout)
        cling_ = _popen(("\"" + cmd + "\"").c_str(), "w");
        if (!cling_) {
            std::cerr << "Error: Cannot start cling" << std::endl;
            return;
        }

        // Send preamble
        send("#include <cpct/types.h>");
        send("#include <cpct/io.h>");
        send("#include <memory>");
        send("#include <utility>");
        send("using namespace cpct;");

        std::cout << "C% JIT REPL v0.1.0 (powered by Cling)" << std::endl;
        std::cout << "Type 'exit' to quit." << std::endl;
        std::cout << std::endl;

        // REPL loop
        std::string line;
        while (true) {
            std::cout << "c%> ";
            std::cout.flush();
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            if (line.empty()) continue;

            // Multi-line: track braces
            std::string source = line;
            int braces = 0;
            for (char c : source) {
                if (c == '{') braces++;
                if (c == '}') braces--;
            }
            while (braces > 0) {
                std::cout << "...  ";
                std::cout.flush();
                if (!std::getline(std::cin, line)) break;
                source += "\n" + line;
                for (char c : line) {
                    if (c == '{') braces++;
                    if (c == '}') braces--;
                }
            }

            // Translate C% → C++ and send to cling
            processInput(source);
        }

        // Cleanup
        send(".q");
        _pclose(cling_);
        std::cout << "Bye!" << std::endl;
    }

private:
    std::string clingPath_;
    std::string resourceDir_;
    FILE* cling_ = nullptr;
    std::vector<std::string> funcDecls_;

    void processInput(const std::string& cpcLine) {
        std::string trimmed = cpcLine;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
            trimmed = trimmed.substr(1);
        if (trimmed.empty()) return;

        // Accumulate function declarations
        if (isFuncDecl(trimmed)) {
            funcDecls_.push_back(cpcLine);
        }

        // Build full source
        std::string fullSource;
        for (auto& f : funcDecls_) fullSource += f + "\n";
        if (!isFuncDecl(trimmed)) {
            fullSource += cpcLine + "\n";
        }

        // Translate
        std::string cppCode;
        try {
            Translator translator;
            cppCode = translator.translate(fullSource);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            if (isFuncDecl(trimmed)) funcDecls_.pop_back();
            return;
        }

        // Extract body (remove preamble and main wrapper)
        std::string body = extractBody(cppCode);
        if (!body.empty()) {
            send(body);
        }
    }

    std::string extractBody(const std::string& cppCode) const {
        std::istringstream stream(cppCode);
        std::string line;
        std::string result;
        bool inMain = false;
        int braceDepth = 0;

        while (std::getline(stream, line)) {
            // Skip preamble lines
            if (line.find("#include") == 0) continue;
            if (line.find("using namespace") == 0) continue;
            if (line.find("// Generated") == 0) continue;

            // Detect "int main() {" or "int main()" followed by "{"
            if (line.find("int main()") != std::string::npos) {
                inMain = true;
                braceDepth = 0;
                // Count braces on the same line (e.g., "int main() {")
                for (char c : line) {
                    if (c == '{') braceDepth++;
                    if (c == '}') braceDepth--;
                }
                continue;
            }

            if (inMain) {
                // Count braces first
                for (char c : line) {
                    if (c == '{') braceDepth++;
                    if (c == '}') braceDepth--;
                }
                // If braces closed, we're out of main
                if (braceDepth <= 0) { inMain = false; continue; }

                // Remove indentation
                std::string trimmed = line;
                while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
                    trimmed = trimmed.substr(1);
                if (trimmed == "return 0;") continue;
                if (trimmed.empty()) continue;
                result += trimmed + "\n";
            } else {
                // Function declarations outside main
                if (!line.empty()) result += line + "\n";
            }
        }
        return result;
    }

    bool isFuncDecl(const std::string& line) const {
        static const std::vector<std::string> types = {
            "int ", "int8 ", "int16 ", "int32 ", "int64 ",
            "uint ", "uint8 ", "uint16 ", "uint32 ", "uint64 ",
            "float ", "float32 ", "float64 ",
            "string ", "bool ", "char ", "void "
        };
        for (auto& t : types) {
            if (line.rfind(t, 0) == 0 && line.find('(') != std::string::npos) {
                if (line.find('{') != std::string::npos || line.back() == '{') {
                    return true;
                }
            }
        }
        return false;
    }

    void send(const std::string& code) {
        if (!cling_) return;
        // Send each line separately to cling
        std::istringstream stream(code);
        std::string line;
        while (std::getline(stream, line)) {
            fprintf(cling_, "%s\n", line.c_str());
            fflush(cling_);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string home = getenv("USERPROFILE") ? getenv("USERPROFILE") : "";
    std::string condaEnv = home + "/.conda/envs/cpct-cling/Library";
    std::string clingPath = condaEnv + "/bin/cling.exe";
    std::string resourceDir = condaEnv + "/lib/clang/18";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--cling-path" && i + 1 < argc) clingPath = argv[++i];
        else if (arg == "--resource-dir" && i + 1 < argc) resourceDir = argv[++i];
    }

    if (!fs::exists(clingPath)) {
        std::cerr << "Error: cling not found at " << clingPath << std::endl;
        std::cerr << "Install: conda create -n cpct-cling -c conda-forge cling" << std::endl;
        return 1;
    }

    ClingRepl repl(clingPath, resourceDir);
    repl.run();
    return 0;
}
