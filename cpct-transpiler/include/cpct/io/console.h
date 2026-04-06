// cpct/io/console.h
// C% I/O functions — print, println, input.
// Uses std::cout for output, std::cin for input.
#pragma once
#include <iostream>
#include <string>

namespace cpct {

// print(args...) — outputs all arguments with no separator, no newline.
template<typename... Args>
void print(Args&&... args) {
    (std::cout << ... << std::forward<Args>(args));
}

// println(args...) — outputs all arguments with no separator, then newline.
template<typename... Args>
void println(Args&&... args) {
    (std::cout << ... << std::forward<Args>(args));
    std::cout << '\n';
}

// println() — outputs just a newline.
inline void println() {
    std::cout << '\n';
}

// input() — reads one line from stdin, returns as std::string.
inline std::string input() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// input(prompt) — prints prompt then reads one line.
template<typename T>
std::string input(const T& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

} // namespace cpct
