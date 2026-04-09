// cpct — unified C% entry point.
// Delegates to cpct-jit, cpct-translate, cpct-compile based on options.
#include "core/version.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
static std::wstring exeDirW; // wide string for Windows Unicode paths

static int run(const std::string& exe, const std::string& args = "") {
    // Build wide command line using exeDirW
    std::wstring wexe(exe.begin(), exe.end());
    std::wstring wext(L".exe");
    std::wstring wcmd = exeDirW + wexe + wext;
    if (!args.empty()) {
        wcmd += L" ";
        std::wstring wargs(args.begin(), args.end());
        wcmd += wargs;
    }

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(NULL, &wcmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        // Fallback to system()
        std::string cmd = exe + ".exe";
        if (!args.empty()) cmd += " " + args;
        return std::system(cmd.c_str());
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}
#else
static std::string exeDir;

static int run(const std::string& exe, const std::string& args = "") {
    std::string full = exeDir + exe;
    std::string cmd;
    if (std::ifstream(full).good()) cmd = full;
    else cmd = exe;
    if (!args.empty()) cmd += " " + args;
    return std::system(cmd.c_str());
}
#endif

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
#ifdef _WIN32
    wchar_t wpath[MAX_PATH];
    GetModuleFileNameW(NULL, wpath, MAX_PATH);
    exeDirW = wpath;
    auto ls = exeDirW.find_last_of(L"/\\");
    if (ls != std::wstring::npos) exeDirW = exeDirW.substr(0, ls + 1);
    else exeDirW = L"";
#else
    std::string exePath = argv[0];
    auto ls = exePath.find_last_of("/\\");
    exeDir = (ls != std::string::npos) ? exePath.substr(0, ls + 1) : "";
#endif

    if (argc < 2) return run("cpct-jit");

    std::string first = argv[1];

    if (first == "--version" || first == "-v") {
        std::cout << "cpct v" CPCT_VERSION;
        if (CPCT_NIGHTLY) std::cout << "-nightly-" CPCT_BUILD_TIME "-" CPCT_GIT_HASH;
        std::cout << std::endl;
        return 0;
    }
    if (first == "--help" || first == "-h") { printUsage(); return 0; }

    auto buildArgs = [&](int start) {
        std::string args;
        for (int i = start; i < argc; i++) { if (!args.empty()) args += " "; args += argv[i]; }
        return args;
    };

    if (first == "--translate") return run("cpct-translate", buildArgs(2));
    if (first == "--compile")  return run("cpct-compile", buildArgs(2));
    if (first == "--run")      return run("cpct-compile", buildArgs(2) + " --run");

    return run("cpct-jit", buildArgs(1));
}
