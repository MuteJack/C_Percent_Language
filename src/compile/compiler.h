// compile/compiler.h — C% compiler: translates .cpc to C++, then compiles with g++/cl.
#pragma once
#include "../translate/translator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace cpctc {

#ifdef _WIN32
static const char* EXE_EXT = ".exe";
#else
static const char* EXE_EXT = "";
#endif

// Find lib path (src/lib or tools/cling-build/include)
inline std::string findLibPath(const std::string& exeBaseDir = "") {
    std::vector<std::string> candidates = {
        exeBaseDir + "tools/cling-build/include",
        exeBaseDir + "src/lib",
        "tools/cling-build/include",
        "src/lib",
        "../src/lib",
    };
    for (auto& p : candidates) {
        if (fs::exists(p + "/cpct/types.h")) return fs::canonical(p).string();
    }
    return "src/lib";
}

// Read sources.txt and return space-separated .cpp paths
inline std::string readLibSources(const std::string& libPath) {
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

// Read file contents
inline std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Write file contents
inline bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

// Translate .cpc source to C++ (returns empty string on error)
inline std::string translateSource(const std::string& cpcSource) {
    try {
        Translator t;
        return t.translate(cpcSource);
    } catch (const std::exception& e) {
        std::cerr << "Translation error: " << e.what() << std::endl;
        return "";
    }
}

// Translate a .cpc file and write .cpp output
// outputDir: directory for output (default: ./translated/)
// Returns 0 on success
inline int translateFile(const std::string& inputFile, const std::string& outputDir = "./translated/") {
    std::string source = readFile(inputFile);
    if (source.empty()) {
        std::cerr << "Error: Cannot read '" << inputFile << "'" << std::endl;
        return 1;
    }

    std::string cppCode = translateSource(source);
    if (cppCode.empty()) return 1;

    fs::create_directories(outputDir);
    std::string outFile = (fs::path(outputDir) / fs::path(inputFile).stem()).string() + ".cpp";

    if (!writeFile(outFile, cppCode)) {
        std::cerr << "Error: Cannot write '" << outFile << "'" << std::endl;
        return 1;
    }

    std::cerr << "Translated: " << inputFile << " -> " << outFile << std::endl;
    return 0;
}

// Compile a .cpc file to executable
// outputDir: directory for output (default: ./_build/)
// Returns 0 on success
inline int compileFile(const std::string& inputFile, const std::string& outputDir = "./_build/",
                       const std::string& exeBaseDir = "") {
    std::string source = readFile(inputFile);
    if (source.empty()) {
        std::cerr << "Error: Cannot read '" << inputFile << "'" << std::endl;
        return 1;
    }

    std::string cppCode = translateSource(source);
    if (cppCode.empty()) return 1;

    // Write to temp file
    std::string tempCpp = (fs::temp_directory_path() / "_cpct_tmp.cpp").string();
    writeFile(tempCpp, cppCode);

    // Find lib path and sources
    std::string libPath = findLibPath(exeBaseDir);
    std::string libSources = readLibSources(libPath);

    // Create output directory
    fs::create_directories(outputDir);
    std::string outputExe = (fs::path(outputDir) / fs::path(inputFile).stem()).string() + EXE_EXT;

    // Build compile command
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
    return 0;
}

// Compile and run a .cpc file
inline int compileAndRun(const std::string& inputFile, const std::string& exeBaseDir = "") {
    std::string source = readFile(inputFile);
    if (source.empty()) {
        std::cerr << "Error: Cannot read '" << inputFile << "'" << std::endl;
        return 1;
    }

    std::string cppCode = translateSource(source);
    if (cppCode.empty()) return 1;

    std::string tempCpp = (fs::temp_directory_path() / "_cpct_tmp.cpp").string();
    std::string tempExe = (fs::temp_directory_path() / ("_cpct_tmp" + std::string(EXE_EXT))).string();
    writeFile(tempCpp, cppCode);

    std::string libPath = findLibPath(exeBaseDir);
    std::string libSources = readLibSources(libPath);

    std::string compileCmd = "g++ -std=c++20 -Wno-sign-compare"
                             " -I \"" + libPath + "\""
                             " \"" + tempCpp + "\""
                             " " + libSources +
                             " -o \"" + tempExe + "\"";

    int ret = std::system(compileCmd.c_str());
    std::remove(tempCpp.c_str());

    if (ret != 0) {
        std::cerr << "Compilation failed." << std::endl;
        return 1;
    }

    ret = std::system(("\"" + tempExe + "\"").c_str());
    std::remove(tempExe.c_str());
    return ret;
}

} // namespace cpctc
