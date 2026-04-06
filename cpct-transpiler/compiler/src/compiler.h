// compiler.h
// Build manager — invokes g++/clang++ to compile translated C++ source.
// Manages include paths, compiler flags, and platform-specific settings.
#pragma once
#include <string>
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>

class Compiler {
public:
    struct Config {
        std::string compiler = "g++";           // g++, clang++, etc.
        std::string standard = "c++20";         // C++ standard
        std::string includePath;                // path to cpct/include
        std::string outputPath = "a.out";       // output binary
        std::vector<std::string> extraFlags;    // additional compiler flags
        bool optimize = false;                  // -O2
        bool warnings = true;                   // -Wall -Wextra
    };

    explicit Compiler(const Config& config) : config_(config) {}

    // Compiles a .cpp file and returns the exit code.
    int compile(const std::string& cppFile) const {
        std::string cmd = buildCommand(cppFile);
        return std::system(cmd.c_str());
    }

    // Compiles and runs the output binary. Returns exit code.
    int compileAndRun(const std::string& cppFile) const {
        int ret = compile(cppFile);
        if (ret != 0) return ret;
        return std::system(config_.outputPath.c_str());
    }

    // Returns the command that would be executed (for debugging).
    std::string getCommand(const std::string& cppFile) const {
        return buildCommand(cppFile);
    }

private:
    Config config_;

    std::string buildCommand(const std::string& cppFile) const {
        std::string cmd = config_.compiler;
        cmd += " -std=" + config_.standard;

        if (config_.warnings) {
            cmd += " -Wall -Wextra";
        }
        if (config_.optimize) {
            cmd += " -O2";
        }

        if (!config_.includePath.empty()) {
            cmd += " -I \"" + config_.includePath + "\"";
        }

        for (auto& flag : config_.extraFlags) {
            cmd += " " + flag;
        }

        cmd += " \"" + cppFile + "\"";
        cmd += " -o \"" + config_.outputPath + "\"";

        return cmd;
    }
};
