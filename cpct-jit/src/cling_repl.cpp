// cpct CLI — unified C% entry point.
// Modes:
//   cpct                              REPL (JIT via Cling)
//   cpct script.cpc                   JIT execute file
//   cpct --translate script.cpc       C% → C++ (calls translate.exe)
//   cpct --compile script.cpc -o out  Translate + compile (calls compile.exe)
//   cpct --run script.cpc             Compile + run (calls compile.exe --run)
//   cpct --version / --help
#include "../../cpct-transpiler/translator/src/translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

// ============== JIT REPL (Cling) ==============

class ClingRepl {
public:
    ClingRepl(const std::string& clingPath, const std::string& resourceDir)
        : clingPath_(clingPath), resourceDir_(resourceDir) {}

    void run() {
        std::string cmd = clingPath_ + " --nologo"
                          " -resource-dir \"" + resourceDir_ + "\""
                          " -std=c++20";

        cling_ = _popen(("\"" + cmd + "\"").c_str(), "w");
        if (!cling_) {
            std::cerr << "Error: Cannot start cling" << std::endl;
            return;
        }

        send("#include <cpct/types.h>");
        send("#include <cpct/io.h>");
        send("#include <memory>");
        send("#include <utility>");
        send("using namespace cpct;");

        std::cout << "C% v0.1.0 (JIT via Cling)" << std::endl;
        std::cout << "Type 'exit' to quit." << std::endl;
        std::cout << std::endl;

        std::string line;
        while (true) {
            std::cout << "c%> ";
            std::cout.flush();
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            if (line.empty()) continue;

            std::string source = line;
            int braces = 0;
            for (char c : source) { if (c == '{') braces++; if (c == '}') braces--; }
            while (braces > 0) {
                std::cout << "...  ";
                std::cout.flush();
                if (!std::getline(std::cin, line)) break;
                source += "\n" + line;
                for (char c : line) { if (c == '{') braces++; if (c == '}') braces--; }
            }

            processInput(source);
        }

        send(".q");
        _pclose(cling_);
        std::cout << "Bye!" << std::endl;
    }

    // Execute a file via JIT
    void runFile(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.is_open()) {
            std::cerr << "Error: Cannot open file '" << filepath << "'" << std::endl;
            return;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::string source = ss.str();

        // Translate entire file
        std::string cppCode;
        try {
            Translator translator;
            cppCode = translator.translate(source);
        } catch (const std::exception& e) {
            std::cerr << "Translation error: " << e.what() << std::endl;
            return;
        }

        std::string body = extractBody(cppCode);
        if (body.empty()) return;

        // Start cling
        std::string cmd = clingPath_ + " --nologo"
                          " -resource-dir \"" + resourceDir_ + "\""
                          " -std=c++20";

        cling_ = _popen(("\"" + cmd + "\"").c_str(), "w");
        if (!cling_) {
            std::cerr << "Error: Cannot start cling" << std::endl;
            return;
        }

        send("#include <cpct/types.h>");
        send("#include <cpct/io.h>");
        send("#include <memory>");
        send("#include <utility>");
        send("using namespace cpct;");
        send(body);
        send(".q");
        _pclose(cling_);
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

        if (isFuncDecl(trimmed)) funcDecls_.push_back(cpcLine);

        std::string fullSource;
        for (auto& f : funcDecls_) fullSource += f + "\n";
        if (!isFuncDecl(trimmed)) fullSource += cpcLine + "\n";

        std::string cppCode;
        try {
            Translator translator;
            cppCode = translator.translate(fullSource);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            if (isFuncDecl(trimmed)) funcDecls_.pop_back();
            return;
        }

        std::string body = extractBody(cppCode);
        if (!body.empty()) send(body);
    }

    std::string extractBody(const std::string& cppCode) const {
        std::istringstream stream(cppCode);
        std::string line;
        std::string result;
        bool inMain = false;
        int braceDepth = 0;

        while (std::getline(stream, line)) {
            if (line.find("#include") == 0) continue;
            if (line.find("using namespace") == 0) continue;
            if (line.find("// Generated") == 0) continue;

            if (line.find("int main()") != std::string::npos) {
                inMain = true;
                braceDepth = 0;
                for (char c : line) { if (c == '{') braceDepth++; if (c == '}') braceDepth--; }
                continue;
            }

            if (inMain) {
                for (char c : line) { if (c == '{') braceDepth++; if (c == '}') braceDepth--; }
                if (braceDepth <= 0) { inMain = false; continue; }
                std::string trimmed = line;
                while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
                    trimmed = trimmed.substr(1);
                if (trimmed == "return 0;") continue;
                if (trimmed.empty()) continue;
                result += trimmed + "\n";
            } else {
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
                if (line.find('{') != std::string::npos || line.back() == '{') return true;
            }
        }
        return false;
    }

    void send(const std::string& code) {
        if (!cling_) return;
        std::istringstream stream(code);
        std::string line;
        while (std::getline(stream, line)) {
            fprintf(cling_, "%s\n", line.c_str());
            fflush(cling_);
        }
    }
};

// ============== Helper: find executables ==============

static std::string exeDir; // set in main()

// Run an external command, bypassing cmd.exe path issues
static int runCommand(const std::string& cmd) {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::string cmdCopy = cmd; // CreateProcessA needs non-const
    if (!CreateProcessA(NULL, (LPSTR)cmdCopy.c_str(), NULL, NULL, TRUE,
                        0, NULL, NULL, &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}

static std::string findExe(const std::string& name) {
    std::string local = exeDir + name;
    if (std::ifstream(local).good()) return local;
    return name;
}

static std::string findCling() {
    std::string home = getenv("USERPROFILE") ? getenv("USERPROFILE") : "";
    return home + "/.conda/envs/cpct-cling/Library/bin/cling.exe";
}

static std::string findClingResourceDir() {
    std::string home = getenv("USERPROFILE") ? getenv("USERPROFILE") : "";
    return home + "/.conda/envs/cpct-cling/Library/lib/clang/18";
}

// ============== Main ==============

static void printUsage() {
    std::cout << "Usage: cpct [options] [script.cpc]" << std::endl;
    std::cout << "  cpct                              REPL mode" << std::endl;
    std::cout << "  cpct script.cpc                   JIT execute file" << std::endl;
    std::cout << "  cpct --translate script.cpc [out]  Translate to C++" << std::endl;
    std::cout << "  cpct --compile script.cpc -o out   Compile to executable" << std::endl;
    std::cout << "  cpct --run script.cpc              Compile and run" << std::endl;
    std::cout << "  cpct --version                     Show version" << std::endl;
    std::cout << "  cpct --help                        Show this help" << std::endl;
}

int main(int argc, char* argv[]) {
    // Determine directory of this executable
    std::string exePath = argv[0];
    auto lastSlash = exePath.find_last_of("/\\");
    exeDir = (lastSlash != std::string::npos) ? exePath.substr(0, lastSlash + 1) : "";

    // No args → REPL
    if (argc < 2) {
        std::string clingPath = findCling();
        if (!fs::exists(clingPath)) {
            std::cerr << "Error: cling not found at " << clingPath << std::endl;
            std::cerr << "Install: conda create -n cpct-cling -c conda-forge cling" << std::endl;
            return 1;
        }
        ClingRepl repl(clingPath, findClingResourceDir());
        repl.run();
        return 0;
    }

    std::string first = argv[1];

    // --version
    if (first == "--version" || first == "-v") {
        std::cout << "cpct v0.1.0" << std::endl;
        return 0;
    }

    // --help
    if (first == "--help" || first == "-h") {
        printUsage();
        return 0;
    }

    // --translate: delegate to translate.exe
    if (first == "--translate") {
        std::string exe = findExe("translate.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        return runCommand(cmd);
    }

    // --compile: delegate to compile.exe
    if (first == "--compile") {
        std::string exe = findExe("compile.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        return runCommand(cmd);
    }

    // --run: delegate to compile.exe --run
    if (first == "--run") {
        std::string exe = findExe("compile.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        cmd += " --run";
        return runCommand(cmd);
    }

    // Default: JIT execute file
    std::string clingPath = findCling();
    if (!fs::exists(clingPath)) {
        std::cerr << "Error: cling not found. Install: conda create -n cpct-cling -c conda-forge cling" << std::endl;
        return 1;
    }
    ClingRepl repl(clingPath, findClingResourceDir());
    repl.runFile(first);
    return 0;
}
