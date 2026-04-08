// cpct-jit.exe — C% JIT REPL via Cling.
// Usage:
//   cpct-jit                 REPL mode
//   cpct-jit script.cpc      JIT execute file
#include "../translate/translator.h"
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
        std::string cmd = clingPath_ + " --nologo"
                          " -resource-dir \"" + resourceDir_ + "\""
                          " -std=c++20";
        cling_ = _popen(("\"" + cmd + "\"").c_str(), "w");
        if (!cling_) { std::cerr << "Error: Cannot start cling" << std::endl; return; }

        send("#include \"cling/Interpreter/RuntimeOptions.h\"");
        send("cling::runtime::gClingOpts->AllowRedefinition = 0;");
        send("#include <cpct/types.h>");
        send("#include <cpct/io.h>");
        send("#include <memory>");
        send("#include <utility>");
        send("using namespace cpct;");

        std::cout << "C% v0.1.0 (JIT via Cling)" << std::endl;
        std::cout << "Type 'exit' to quit." << std::endl << std::endl;

        std::string line;
        while (true) {
            std::cout << "c%> "; std::cout.flush();
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            if (line.empty()) continue;
            std::string source = line;
            int braces = 0;
            for (char c : source) { if (c == '{') braces++; if (c == '}') braces--; }
            while (braces > 0) {
                std::cout << "...  "; std::cout.flush();
                if (!std::getline(std::cin, line)) break;
                source += "\n" + line;
                for (char c : line) { if (c == '{') braces++; if (c == '}') braces--; }
            }
            processInput(source);
        }
        send(".q"); _pclose(cling_);
        std::cout << "Bye!" << std::endl;
    }

    void runFile(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.is_open()) { std::cerr << "Error: Cannot open '" << filepath << "'" << std::endl; return; }
        std::stringstream ss; ss << f.rdbuf();
        std::string cppCode;
        try { Translator t; cppCode = t.translate(ss.str()); }
        catch (const std::exception& e) { std::cerr << "Error: " << e.what() << std::endl; return; }
        std::string body = extractBody(cppCode);
        if (body.empty()) return;
        std::string cmd = clingPath_ + " --nologo -resource-dir \"" + resourceDir_ + "\" -std=c++20";
        cling_ = _popen(("\"" + cmd + "\"").c_str(), "w");
        if (!cling_) { std::cerr << "Error: Cannot start cling" << std::endl; return; }
        send("#include \"cling/Interpreter/RuntimeOptions.h\"");
        send("cling::runtime::gClingOpts->AllowRedefinition = 0;");
        send("#include <cpct/types.h>"); send("#include <cpct/io.h>");
        send("#include <memory>"); send("#include <utility>");
        send("using namespace cpct;");
        send(body); send(".q"); _pclose(cling_);
    }

private:
    std::string clingPath_, resourceDir_;
    FILE* cling_ = nullptr;
    std::vector<std::string> funcDecls_;

    void processInput(const std::string& cpcLine) {
        std::string trimmed = cpcLine;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed = trimmed.substr(1);
        if (trimmed.empty()) return;
        if (isFuncDecl(trimmed)) funcDecls_.push_back(cpcLine);
        std::string full;
        for (auto& f : funcDecls_) full += f + "\n";
        if (!isFuncDecl(trimmed)) full += cpcLine + "\n";
        std::string cppCode;
        try { Translator t; cppCode = t.translate(full); }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            if (isFuncDecl(trimmed)) funcDecls_.pop_back();
            return;
        }
        std::string body = extractBody(cppCode);
        if (!body.empty()) send(body);
    }

    std::string extractBody(const std::string& cppCode) const {
        std::istringstream stream(cppCode); std::string line, result;
        bool inMain = false; int depth = 0;
        while (std::getline(stream, line)) {
            if (line.find("#include") == 0 || line.find("using namespace") == 0 || line.find("// Generated") == 0) continue;
            if (line.find("int main()") != std::string::npos) {
                inMain = true; depth = 0;
                for (char c : line) { if (c == '{') depth++; if (c == '}') depth--; }
                continue;
            }
            if (inMain) {
                for (char c : line) { if (c == '{') depth++; if (c == '}') depth--; }
                if (depth <= 0) { inMain = false; continue; }
                std::string t = line;
                while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t = t.substr(1);
                if (t == "return 0;" || t.empty()) continue;
                result += t + "\n";
            } else { if (!line.empty()) result += line + "\n"; }
        }
        return result;
    }

    bool isFuncDecl(const std::string& line) const {
        static const std::vector<std::string> types = {
            "int ","int8 ","int16 ","int32 ","int64 ","uint ","uint8 ","uint16 ","uint32 ","uint64 ",
            "float ","float32 ","float64 ","string ","bool ","char ","void "};
        for (auto& t : types)
            if (line.rfind(t, 0) == 0 && line.find('(') != std::string::npos &&
                (line.find('{') != std::string::npos || line.back() == '{')) return true;
        return false;
    }

    void send(const std::string& code) {
        if (!cling_) return;
        std::istringstream s(code); std::string line;
        while (std::getline(s, line)) { fprintf(cling_, "%s\n", line.c_str()); fflush(cling_); }
    }
};

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string a = argv[1];
        if (a == "--version" || a == "-v") { std::cout << "cpct-jit v0.1.0" << std::endl; return 0; }
        if (a == "--help" || a == "-h") { std::cout << "Usage: cpct-jit [script.cpc]" << std::endl; return 0; }
    }
    std::string home = getenv("USERPROFILE") ? getenv("USERPROFILE") : "";
    std::string clingPath = home + "/.conda/envs/cpct-cling/Library/bin/cling.exe";
    std::string resourceDir = home + "/.conda/envs/cpct-cling/Library/lib/clang/18";
    if (!fs::exists(clingPath)) {
        std::cerr << "Error: cling not found. Install: conda create -n cpct-cling -c conda-forge cling" << std::endl;
        return 1;
    }
    ClingRepl repl(clingPath, resourceDir);
    if (argc >= 2) repl.runFile(argv[1]); else repl.run();
    return 0;
}
