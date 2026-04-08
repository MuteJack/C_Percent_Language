// C% Compiler CLI — translates .cpc to C++ via cpct-translate, then compiles with g++.
// Usage:
//   cpct-compile input.cpc -o output      Translate + compile
//   cpct-compile input.cpc --run          Translate + compile + run
//   cpct-compile input.cpc --emit-cpp     Also keep the generated .cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <windows.h>

namespace fs = std::filesystem;

// Find cpct-translate.exe relative to working directory
static std::string findTranslator() {
    // Get directory of current executable
    std::string dir = fs::path(fs::current_path()).string();
    std::string local = dir + "\\cpct-translate.exe";
    if (std::ifstream(local).good()) return local;
    // Try without path (rely on PATH)
    return "cpct-translate.exe";
}

// Find cpct-cpp-lib path
static std::string findLibPath() {
    std::vector<std::string> candidates = {
        "cpct-cpp-lib",
        "../cpct-cpp-lib",
        "../../cpct-cpp-lib",
    };
    for (auto& p : candidates) {
        if (std::ifstream(p + "/cpct/types.h").good()) return p;
    }
    return "cpct-cpp-lib";
}

// Read sources.txt and return space-separated .cpp paths
static std::string readLibSources(const std::string& libPath) {
    std::ifstream f(libPath + "/sources.txt");
    if (!f.is_open()) return "";

    std::string result;
    std::string line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.pop_back();
        if (line.empty()) continue;
        if (!result.empty()) result += " ";
        result += "\"" + libPath + "/" + line + "\"";
    }
    return result;
}

static void printUsage() {
    std::cerr << "Usage: cpct-compile <input.cpc> [options]" << std::endl;
    std::cerr << "  cpct-compile input.cpc -o output      Translate + compile" << std::endl;
    std::cerr << "  cpct-compile input.cpc --run           Translate + compile + run" << std::endl;
    std::cerr << "  cpct-compile input.cpc --emit-cpp      Also keep the generated .cpp" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string inputFile;
    std::string outputExe;
    bool runAfter = false;
    bool emitCpp = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputExe = argv[++i];
        } else if (arg == "--run") {
            runAfter = true;
        } else if (arg == "--emit-cpp") {
            emitCpp = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (inputFile.empty()) {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        printUsage();
        return 1;
    }

    if (outputExe.empty() && !runAfter) {
        std::cerr << "Error: specify -o <output> or --run" << std::endl;
        printUsage();
        return 1;
    }

    // 1. Translate: cpct-translate.exe input.cpc temp.cpp
    std::string translator = findTranslator();
    std::string tempCpp = (fs::temp_directory_path() / "_cpct_tmp.cpp").string();

    // Translate using cpct-translate — capture stdout via pipe
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {};
    std::string translateCmd = translator + " " + inputFile;
    if (!CreateProcessA(NULL, (LPSTR)translateCmd.c_str(), NULL, NULL, TRUE,
                        0, NULL, NULL, &si, &pi)) {
        std::cerr << "Translation failed: cannot run cpct-translate" << std::endl;
        return 1;
    }
    CloseHandle(hWritePipe);

    // Read translated C++ from pipe
    std::string cppCode;
    char buf[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        cppCode += buf;
    }
    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    int ret;
    if (exitCode != 0 || cppCode.empty()) {
        std::cerr << "Translation failed." << std::endl;
        return 1;
    }
    // Write translated code to temp file
    {
        std::ofstream f(tempCpp);
        f << cppCode;
    }

    // Emit .cpp if requested
    if (emitCpp) {
        std::string emitPath = outputExe.empty() ?
            (fs::path(inputFile).stem().string() + ".cpp") :
            (outputExe + ".cpp");
        fs::copy_file(tempCpp, emitPath, fs::copy_options::overwrite_existing);
        std::cerr << "Emitted: " << emitPath << std::endl;
    }

    // 2. Compile: g++ temp.cpp -o output.exe
    bool isTempExe = false;
    if (outputExe.empty()) {
        outputExe = (fs::temp_directory_path() / "_cpct_tmp.exe").string();
        isTempExe = true;
    }

    std::string libPath = findLibPath();
    std::string libSources = readLibSources(libPath);

    std::string compileCmd = "g++ -std=c++20 -Wno-sign-compare"
                             " -I \"" + libPath + "\""
                             " \"" + tempCpp + "\""
                             " " + libSources +
                             " -o \"" + outputExe + "\"";

    ret = std::system(compileCmd.c_str());
    std::remove(tempCpp.c_str());

    if (ret != 0) {
        std::cerr << "Compilation failed." << std::endl;
        return 1;
    }

    std::cerr << "Compiled: " << inputFile << " -> " << outputExe << std::endl;

    // 3. Run if requested
    if (runAfter) {
        ret = std::system(("\"" + outputExe + "\"").c_str());
        if (isTempExe) {
            std::remove(outputExe.c_str());
        }
        return ret;
    }

    return 0;
}
