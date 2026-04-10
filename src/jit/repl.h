// jit/repl.h — C% JIT REPL via Cling.
// ClingProcess: bidirectional pipe to cling with [cling]$ prompt filtering.
// ClingRepl: interactive REPL and file execution.
#pragma once
#include "../translate/translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

// Bidirectional pipe to cling process with stdout prompt filtering.
class ClingProcess {
public:
    bool start(const std::string& cmd) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hStdinRd, hStdinWr, hStdoutRd, hStdoutWr;
        if (!CreatePipe(&hStdinRd, &hStdinWr, &sa, 0)) return false;
        if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
            CloseHandle(hStdinRd); CloseHandle(hStdinWr);
            return false;
        }
        SetHandleInformation(hStdinWr, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = hStdinRd;
        si.hStdOutput = hStdoutWr;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        std::string cmdLine = cmd;
        if (!CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                            CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi_)) {
            CloseHandle(hStdinRd); CloseHandle(hStdinWr);
            CloseHandle(hStdoutRd); CloseHandle(hStdoutWr);
            return false;
        }
        CloseHandle(hStdinRd);
        CloseHandle(hStdoutWr);
        hStdinWr_ = hStdinWr;
        hStdoutRd_ = hStdoutRd;
#else
        int stdinPipe[2], stdoutPipe[2];
        if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0) return false;

        pid_ = fork();
        if (pid_ < 0) return false;
        if (pid_ == 0) {
            setpgid(0, 0);
            ::close(stdinPipe[1]);
            ::close(stdoutPipe[0]);
            dup2(stdinPipe[0], STDIN_FILENO);
            dup2(stdoutPipe[1], STDOUT_FILENO);
            ::close(stdinPipe[0]);
            ::close(stdoutPipe[1]);
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            _exit(127);
        }
        ::close(stdinPipe[0]);
        ::close(stdoutPipe[1]);
        stdinFd_ = stdinPipe[1];
        stdoutFd_ = stdoutPipe[0];
#endif
        running_ = true;
        readerThread_ = std::thread(&ClingProcess::readerLoop, this);
        return true;
    }

    void write(const std::string& data) {
#ifdef _WIN32
        DWORD written;
        WriteFile(hStdinWr_, data.c_str(), (DWORD)data.size(), &written, nullptr);
#else
        if (::write(stdinFd_, data.c_str(), data.size()) < 0) {}
#endif
    }

    void close() {
#ifdef _WIN32
        if (hStdinWr_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hStdinWr_);
            hStdinWr_ = INVALID_HANDLE_VALUE;
        }
#else
        if (stdinFd_ >= 0) { ::close(stdinFd_); stdinFd_ = -1; }
#endif
        running_ = false;
        if (readerThread_.joinable()) readerThread_.join();
#ifdef _WIN32
        if (pi_.hProcess) {
            WaitForSingleObject(pi_.hProcess, 5000);
            CloseHandle(pi_.hProcess);
            CloseHandle(pi_.hThread);
            pi_ = {};
        }
        if (hStdoutRd_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hStdoutRd_);
            hStdoutRd_ = INVALID_HANDLE_VALUE;
        }
#else
        if (pid_ > 0) { waitpid(pid_, nullptr, 0); pid_ = -1; }
        if (stdoutFd_ >= 0) { ::close(stdoutFd_); stdoutFd_ = -1; }
#endif
    }

    void interrupt() {
#ifdef _WIN32
        if (pi_.dwProcessId) {
            GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pi_.dwProcessId);
        }
#else
        if (pid_ > 0) { kill(pid_, SIGINT); }
#endif
    }

    ~ClingProcess() { close(); }

private:
#ifdef _WIN32
    HANDLE hStdinWr_ = INVALID_HANDLE_VALUE;
    HANDLE hStdoutRd_ = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi_ = {};
#else
    int stdinFd_ = -1;
    int stdoutFd_ = -1;
    pid_t pid_ = -1;
#endif
    std::thread readerThread_;
    std::atomic<bool> running_{false};

    int readPipe(char* buf, int maxLen) {
#ifdef _WIN32
        DWORD n = 0;
        if (!ReadFile(hStdoutRd_, buf, maxLen, &n, nullptr)) return -1;
        return (int)n;
#else
        return (int)::read(stdoutFd_, buf, maxLen);
#endif
    }

    void readerLoop() {
        const std::string PROMPT = "[cling]$ ";
        const std::string PROMPT2 = "[cling]$";
        std::string buf;
        char chunk[4096];

        while (true) {
            int n = readPipe(chunk, sizeof(chunk));
            if (n <= 0) break;
            buf.append(chunk, n);

            std::string::size_type pos;
            while ((pos = buf.find(PROMPT)) != std::string::npos)
                buf.erase(pos, PROMPT.size());
            while ((pos = buf.find(PROMPT2)) != std::string::npos)
                buf.erase(pos, PROMPT2.size());

            size_t safe = buf.size();
            for (size_t len = 1; len < PROMPT.size() && len <= buf.size(); len++) {
                if (PROMPT.compare(0, len, buf, buf.size() - len, len) == 0) {
                    safe = buf.size() - len;
                    break;
                }
            }

            if (safe > 0) {
                std::cout.write(buf.data(), safe);
                std::cout.flush();
                buf.erase(0, safe);
            }
        }

        std::string::size_type pos;
        while ((pos = buf.find(PROMPT)) != std::string::npos)
            buf.erase(pos, PROMPT.size());
        while ((pos = buf.find(PROMPT2)) != std::string::npos)
            buf.erase(pos, PROMPT2.size());
        if (!buf.empty()) { std::cout << buf; std::cout.flush(); }
    }
};

class ClingRepl {
public:
    ClingRepl(const std::string& clingPath, const std::string& resourceDir,
              const std::string& libPath = "")
        : clingPath_(clingPath), resourceDir_(resourceDir), libPath_(libPath) {}

    // Auto-detect cling and lib paths relative to exePath
    static ClingRepl autoDetect(const std::string& exePath) {
        std::string clingPath, resourceDir, libPath;
        auto els = exePath.find_last_of("/\\");
        std::string exeBaseDir = (els != std::string::npos) ? exePath.substr(0, els + 1) : "";

#ifdef _WIN32
        const char* homeVar = "USERPROFILE";
#else
        const char* homeVar = "HOME";
#endif
        std::string home = getenv(homeVar) ? getenv(homeVar) : "";

        struct { std::string path; std::string resDir; } candidates[] = {
            { exeBaseDir + "tools/cling-build/bin/cling" + std::string(
#ifdef _WIN32
              ".exe"
#else
              ""
#endif
            ), exeBaseDir + "tools/cling-build/lib/clang" },
            { home + "/.conda/envs/cpct-cling/Library/bin/cling.exe",
              home + "/.conda/envs/cpct-cling/Library/lib/clang/18" },
            { home + "/.conda/envs/cpct-cling/bin/cling",
              home + "/.conda/envs/cpct-cling/lib/clang/18" },
            { home + "/miniconda3/envs/cpct-cling/bin/cling",
              home + "/miniconda3/envs/cpct-cling/lib/clang/18" },
            { "/usr/bin/cling", "/usr/lib/clang/18" },
        };
        bool found = false;
        for (auto& c : candidates) {
            if (fs::exists(c.path)) {
                clingPath = c.path;
                resourceDir = c.resDir;
                if (fs::is_directory(resourceDir) && !fs::exists(resourceDir + "/include")) {
                    for (auto& entry : fs::directory_iterator(resourceDir)) {
                        if (entry.is_directory()) {
                            resourceDir = entry.path().string();
                            break;
                        }
                    }
                }
                found = true;
                break;
            }
        }
        if (!found) {
#ifdef _WIN32
            if (std::system("cling --version > nul 2>&1") == 0) {
#else
            if (std::system("cling --version > /dev/null 2>&1") == 0) {
#endif
                clingPath = "cling";
            } else {
                std::cerr << "Error: cling not found." << std::endl;
                std::cerr << "Build: bash build.sh cling" << std::endl;
                std::cerr << "Or install via:" << std::endl;
                std::cerr << "  conda: conda create -n cpct-cling -c conda-forge cling" << std::endl;
                std::cerr << "  snap:  sudo snap install cling" << std::endl;
            }
        }

        // Find cpct lib path
        std::vector<std::string> libCandidates = {
            exeBaseDir + "tools/cling-build/include",
            exeBaseDir + "../tools/cling-build/include",
            "tools/cling-build/include",
            exeBaseDir + "src/lib",
            exeBaseDir + "../src/lib",
            "src/lib",
            "../src/lib",
        };
        for (auto& p : libCandidates) {
            if (fs::exists(p + "/cpct/types.h")) { libPath = fs::canonical(p).string(); break; }
        }

        return ClingRepl(clingPath, resourceDir, libPath);
    }

    bool isAvailable() const { return !clingPath_.empty(); }

    void run() {
        if (!proc_.start(buildClingCmd())) {
            std::cerr << "Error: Cannot start cling" << std::endl;
            return;
        }

        sendPreamble();

        std::cout << CPCT_LANG_NAME " v" CPCT_VERSION " (JIT via Cling)" << std::endl;
        std::cout << "Type 'exit' to quit." << std::endl << std::endl;

        std::string line;
        while (true) {
            std::cout << "c%> "; std::cout.flush();
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            if (line.empty()) continue;
            // Backslash continuation
            while (line.size() > 0 && line.back() == '\\') {
                line.pop_back();
                std::cout << "...  "; std::cout.flush();
                std::string next;
                if (!std::getline(std::cin, next)) break;
                line += "\n" + next;
            }
            if (line.empty()) continue;
            std::string source = line;
            int braces = 0;
            for (char c : source) { if (c == '{') braces++; if (c == '}') braces--; }
            while (braces > 0) {
                std::cout << "...  "; std::cout.flush();
                if (!std::getline(std::cin, line)) break;
                // Backslash continuation inside brace block
                while (line.size() > 0 && line.back() == '\\') {
                    line.pop_back();
                    std::cout << "...  "; std::cout.flush();
                    std::string next;
                    if (!std::getline(std::cin, next)) break;
                    line += "\n" + next;
                }
                source += "\n" + line;
                for (char c : line) { if (c == '{') braces++; if (c == '}') braces--; }
            }
            processInput(source);
        }
        send(".q");
        proc_.close();
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

        if (!proc_.start(buildClingCmd())) {
            std::cerr << "Error: Cannot start cling" << std::endl;
            return;
        }
        sendPreamble();
        send(body);
        send(".q");
        proc_.close();
    }

private:
    std::string clingPath_, resourceDir_, libPath_;
    ClingProcess proc_;
    std::vector<std::string> funcDecls_;

    std::string buildClingCmd() const {
        std::string cmd = "\"" + clingPath_ + "\" --nologo -std=c++20";
        if (!resourceDir_.empty()) cmd += " -resource-dir \"" + resourceDir_ + "\"";
        if (!libPath_.empty()) cmd += " -I \"" + libPath_ + "\"";
        return cmd;
    }

    void sendPreamble() {
        send("#include \"cling/Interpreter/RuntimeOptions.h\"");
        send("cling::runtime::gClingOpts->AllowRedefinition = 0;");
        send("#include <cpct/types.h>");
        send("#include <cpct/io.h>");
        send("#include <memory>");
        send("#include <utility>");
        send("using namespace cpct;");
    }

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
        std::istringstream s(code); std::string line;
        while (std::getline(s, line)) { proc_.write(line + "\n"); }
    }
};
