// C% Compiler CLI — translates .cpc to C++ via cpct-translate, then compiles with g++.
// Cross-platform: Windows, Linux, macOS.
#include "../core/version.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
static const char* EXE_EXT = ".exe";
#else
static const char* EXE_EXT = "";
#endif

#ifdef _WIN32
static std::wstring exeDirW;
#endif

// Find cpct-translate relative to this executable (used on Unix)
[[maybe_unused]] static std::string findTranslator() {
    std::string name = std::string("cpct-translate") + EXE_EXT;
    std::vector<std::string> candidates = {
        "./" + name,
        name,
        "../" + name,
    };
    for (auto& p : candidates) {
        if (std::ifstream(p).good()) return p;
    }
    return name;
}

// Find lib path (src/lib)
static std::string findLibPath() {
    std::vector<std::string> candidates = {
        "src/lib",
        "../src/lib",
        "../../src/lib",
    };
    for (auto& p : candidates) {
        if (std::ifstream(p + "/cpct/types.h").good()) return p;
    }
    return "src/lib";
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
#ifdef _WIN32
    wchar_t wpath[MAX_PATH];
    GetModuleFileNameW(NULL, wpath, MAX_PATH);
    exeDirW = wpath;
    auto wls = exeDirW.find_last_of(L"/\\");
    if (wls != std::wstring::npos) exeDirW = exeDirW.substr(0, wls + 1);
    else exeDirW = L"";
#endif

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
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "cpct-compile v" CPCT_VERSION << std::endl;
            return 0;
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

    // 1. Translate: capture stdout from cpct-translate
    std::string cppCode;
    {
#ifdef _WIN32
        // Windows: use CreateProcessW with pipe to handle Unicode paths
        std::wstring wcmd = exeDirW + L"cpct-translate.exe " +
            std::wstring(inputFile.begin(), inputFile.end());

        SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
        HANDLE hReadPipe, hWritePipe;
        CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        PROCESS_INFORMATION pi = {};
        if (!CreateProcessW(NULL, &wcmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            std::cerr << "Translation failed: cannot run cpct-translate" << std::endl;
            CloseHandle(hReadPipe); CloseHandle(hWritePipe);
            return 1;
        }
        CloseHandle(hWritePipe);

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
        if (exitCode != 0 || cppCode.empty()) {
            std::cerr << "Translation failed." << std::endl;
            return 1;
        }
#else
        // Unix: use popen
        std::string translator = findTranslator();
        std::string translateCmd = translator + " \"" + inputFile + "\"";
        FILE* pipe = popen(translateCmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Translation failed: cannot run cpct-translate" << std::endl;
            return 1;
        }
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) cppCode += buf;
        int pret = pclose(pipe);
        if (pret != 0 || cppCode.empty()) {
            std::cerr << "Translation failed." << std::endl;
            return 1;
        }
#endif
    }

    // Write translated code to temp file
    std::string tempCpp = (fs::temp_directory_path() / "_cpct_tmp.cpp").string();
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

    // 2. Compile
    bool isTempExe = false;
    if (outputExe.empty()) {
        outputExe = (fs::temp_directory_path() / ("_cpct_tmp" + std::string(EXE_EXT))).string();
        isTempExe = true;
    }

    std::string libPath = findLibPath();
    std::string libSources = readLibSources(libPath);

    std::string compileCmd = "g++ -std=c++20 -Wno-sign-compare"
                             " -I \"" + libPath + "\""
                             " \"" + tempCpp + "\""
                             " " + libSources +
                             " -o \"" + outputExe + "\"";

    int ret = std::system(compileCmd.c_str());
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
