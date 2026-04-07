// jit_repl.cpp
// C% JIT REPL — translates C% to C++, compiles to DLL, loads and executes.
// Uses the existing Translator + g++ + Windows LoadLibrary.
#include "../../translator/src/translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;

class JitRepl {
public:
    JitRepl(const std::string& includePath)
        : includePath_(includePath), counter_(0) {
        // Create temp directory
        tempDir_ = fs::temp_directory_path() / "cpct_jit";
        fs::create_directories(tempDir_);
    }

    ~JitRepl() {
        // Unload all DLLs
        for (auto& h : handles_) {
#ifdef _WIN32
            FreeLibrary((HMODULE)h);
#else
            dlclose(h);
#endif
        }
        // Cleanup temp files
        try { fs::remove_all(tempDir_); } catch (...) {}
    }

    // Execute one REPL input. Returns true on success.
    bool execute(const std::string& cpcLine) {
        // Skip empty lines
        std::string trimmed = cpcLine;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
            trimmed = trimmed.substr(1);
        if (trimmed.empty()) return true;

        // Check for function declarations — accumulate separately
        if (isFuncDecl(trimmed)) {
            funcDecls_.push_back(cpcLine);
            std::cout << "(function registered)" << std::endl;
            return true;
        }

        // Check if it's a variable declaration — accumulate
        bool isVarDecl = isVarDeclLine(trimmed);
        if (isVarDecl) {
            varDecls_.push_back(cpcLine);
        }

        // Set current execution statement(s)
        statements_.clear();
        statements_.push_back(cpcLine);

        // Build full C% source: func decls + var decls + current statement
        std::string cpcSource = buildSource(isVarDecl);

        // Translate C% → C++
        std::string cppCode;
        try {
            Translator translator;
            cppCode = translator.translate(cpcSource);
        } catch (const std::exception& e) {
            std::cerr << "Translation error: " << e.what() << std::endl;
            if (isVarDecl) varDecls_.pop_back(); // remove failed decl
            return false;
        }

        // Wrap in DLL-exportable function
        std::string dllCode = wrapForDll(cppCode);

        // Write to temp .cpp file
        std::string cppFile = (tempDir_ / ("jit_" + std::to_string(counter_) + ".cpp")).string();
        std::string dllFile = (tempDir_ / ("jit_" + std::to_string(counter_) + ".dll")).string();
        {
            std::ofstream f(cppFile);
            f << dllCode;
        }

        // Compile to DLL
        std::string cmd = "g++ -std=c++20 -shared -Wno-sign-compare"
                          " -I \"" + includePath_ + "\""
                          " -o \"" + dllFile + "\""
                          " \"" + cppFile + "\""
                          " \"" + includePath_ + "/cpct/data/type/bigint.cpp\""
                          " 2>&1";

        std::string compileOutput = execCommand(cmd);
        if (!fs::exists(dllFile)) {
            std::cerr << "Compile error:\n" << compileOutput << std::endl;
            if (isVarDecl) varDecls_.pop_back(); // remove failed decl
            return false;
        }

        // Load and execute DLL
        if (!loadAndRun(dllFile)) {
            statements_.pop_back();
            return false;
        }

        counter_++;
        return true;
    }

private:
    std::string includePath_;
    fs::path tempDir_;
    int counter_;
    std::vector<std::string> funcDecls_;   // accumulated function declarations
    std::vector<std::string> varDecls_;    // accumulated variable declarations (re-initialized each run)
    std::vector<std::string> statements_;  // only the latest statement(s) to execute
    std::vector<void*> handles_;           // loaded DLL handles

    // Simple heuristic: check if line starts with a return type + name + (
    bool isFuncDecl(const std::string& line) const {
        // Matches patterns like: "int foo(", "void bar(", "string func("
        static const std::vector<std::string> types = {
            "int ", "int8 ", "int16 ", "int32 ", "int64 ",
            "uint ", "uint8 ", "uint16 ", "uint32 ", "uint64 ",
            "float ", "float32 ", "float64 ",
            "string ", "bool ", "char ", "void "
        };
        for (auto& t : types) {
            if (line.rfind(t, 0) == 0 && line.find('(') != std::string::npos) {
                // But not a function call (no semicolon before brace)
                if (line.find('{') != std::string::npos || line.back() == '{') {
                    return true;
                }
            }
        }
        return false;
    }

    // Check if a line is a variable declaration
    bool isVarDeclLine(const std::string& line) const {
        static const std::vector<std::string> types = {
            "int ", "int8 ", "int16 ", "int32 ", "int64 ",
            "uint ", "uint8 ", "uint16 ", "uint32 ", "uint64 ",
            "int8f ", "int16f ", "int32f ",
            "uint8f ", "uint16f ", "uint32f ",
            "float ", "float32 ", "float64 ",
            "string ", "bool ", "char ",
            "const ", "static ", "heap ", "let ", "ref ",
            "vector<", "array<", "dict ", "map<",
            "intbig ", "bigint "
        };
        for (auto& t : types) {
            if (line.rfind(t, 0) == 0 && line.find('(') == std::string::npos) {
                return true;
            }
        }
        return false;
    }

    // Build full C% source: func decls + var decls + current statement
    std::string buildSource(bool currentIsVarDecl) const {
        std::string src;
        // Function declarations
        for (auto& f : funcDecls_) src += f + "\n";
        // Variable declarations (all accumulated)
        for (auto& v : varDecls_) src += v + "\n";
        // Current statement (only if not a var decl — var decls are already in varDecls_)
        if (!currentIsVarDecl) {
            for (auto& s : statements_) src += s + "\n";
        }
        return src;
    }

    // Extract the main() body from translated C++ and wrap as DLL export
    std::string wrapForDll(const std::string& cppCode) const {
        // The translator wraps script mode in int main() { ... }
        // We need to replace main() with a DLL-exported function
        std::string code = cppCode;

        // Replace "int main()" with exported function
        std::string funcName = "jit_run_" + std::to_string(counter_);
        size_t mainPos = code.find("int main()");
        if (mainPos != std::string::npos) {
            code.replace(mainPos, 10,
                "extern \"C\" __declspec(dllexport) int " + funcName + "()");
        }

        // Remove "return 0;" at end of main (we don't need it)
        // Keep it actually, it's harmless

        return code;
    }

    bool loadAndRun(const std::string& dllFile) {
#ifdef _WIN32
        HMODULE dll = LoadLibraryA(dllFile.c_str());
        if (!dll) {
            std::cerr << "Failed to load DLL: " << GetLastError() << std::endl;
            return false;
        }
        handles_.push_back((void*)dll);

        std::string funcName = "jit_run_" + std::to_string(counter_);
        typedef int (*RunFunc)();
        RunFunc func = (RunFunc)GetProcAddress(dll, funcName.c_str());
        if (!func) {
            std::cerr << "Failed to find function: " << funcName << std::endl;
            return false;
        }
        func();
        return true;
#else
        void* dll = dlopen(dllFile.c_str(), RTLD_NOW);
        if (!dll) {
            std::cerr << "Failed to load: " << dlerror() << std::endl;
            return false;
        }
        handles_.push_back(dll);

        std::string funcName = "jit_run_" + std::to_string(counter_);
        typedef int (*RunFunc)();
        RunFunc func = (RunFunc)dlsym(dll, funcName.c_str());
        if (!func) {
            std::cerr << "Failed to find function: " << funcName << std::endl;
            return false;
        }
        func();
        return true;
#endif
    }

    // Execute a shell command and capture output
    std::string execCommand(const std::string& cmd) const {
        std::string result;
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe) return "Failed to execute command";
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
        _pclose(pipe);
        return result;
    }
};

int main(int argc, char* argv[]) {
    // Default include path: relative to executable
    std::string includePath = "cpct-transpiler/include";
    if (argc >= 2) {
        includePath = argv[1];
    }

    std::cout << "C% JIT REPL v0.1.0 (translate + compile + DLL)" << std::endl;
    std::cout << "Include path: " << includePath << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;
    std::cout << std::endl;

    JitRepl repl(includePath);
    std::string line;

    while (true) {
        std::cout << "c%> ";
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
            if (!std::getline(std::cin, line)) break;
            source += "\n" + line;
            for (char c : line) {
                if (c == '{') braces++;
                if (c == '}') braces--;
            }
        }

        repl.execute(source);
    }

    std::cout << "Bye!" << std::endl;
    return 0;
}
