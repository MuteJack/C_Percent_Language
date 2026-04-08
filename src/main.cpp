// cpct.exe — unified C% entry point.
// Delegates to cpct-jit, cpct-translate, cpct-compile based on options.
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

static std::string exeDir;

static int runCommand(const std::string& cmd) {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::string cmdCopy = cmd;
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

static void printUsage() {
    std::cout << "Usage: cpct [options] [script.cpc]" << std::endl;
    std::cout << "  cpct                              REPL mode (JIT)" << std::endl;
    std::cout << "  cpct script.cpc                   JIT execute file" << std::endl;
    std::cout << "  cpct --translate script.cpc [out]  Translate to C++" << std::endl;
    std::cout << "  cpct --compile script.cpc -o out   Compile to executable" << std::endl;
    std::cout << "  cpct --run script.cpc              Compile and run" << std::endl;
    std::cout << "  cpct --version                     Show version" << std::endl;
    std::cout << "  cpct --help                        Show this help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string exePath = argv[0];
    auto lastSlash = exePath.find_last_of("/\\");
    exeDir = (lastSlash != std::string::npos) ? exePath.substr(0, lastSlash + 1) : "";

    // No args → REPL (delegate to cpct-jit.exe)
    if (argc < 2) {
        std::string jit = findExe("cpct-jit.exe");
        return runCommand(jit);
    }

    std::string first = argv[1];

    if (first == "--version" || first == "-v") {
        std::cout << "cpct v0.1.0" << std::endl;
        return 0;
    }

    if (first == "--help" || first == "-h") {
        printUsage();
        return 0;
    }

    // --translate: delegate to cpct-translate.exe
    if (first == "--translate") {
        std::string exe = findExe("cpct-translate.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        return runCommand(cmd);
    }

    // --compile: delegate to cpct-compile.exe
    if (first == "--compile") {
        std::string exe = findExe("cpct-compile.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        return runCommand(cmd);
    }

    // --run: delegate to cpct-compile.exe --run
    if (first == "--run") {
        std::string exe = findExe("cpct-compile.exe");
        std::string cmd = exe;
        for (int i = 2; i < argc; i++) { cmd += " "; cmd += argv[i]; }
        cmd += " --run";
        return runCommand(cmd);
    }

    // Default: JIT execute file (delegate to cpct-jit.exe with file arg)
    std::string jit = findExe("cpct-jit.exe");
    std::string cmd = jit;
    for (int i = 1; i < argc; i++) { cmd += " "; cmd += argv[i]; }
    return runCommand(cmd);
}
