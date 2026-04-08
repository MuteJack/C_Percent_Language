// cpct/io/console.h
// C% I/O functions — print, println, input.
// Uses std::cout for output, std::cin for input.
#pragma once
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <any>

// Stream output for std::pair (divmod result)
template<typename A, typename B>
std::ostream& operator<<(std::ostream& os, const std::pair<A, B>& p) {
    return os << "[" << p.first << ", " << p.second << "]";
}

// Stream output for std::any (Dict values)
inline std::ostream& operator<<(std::ostream& os, const std::any& a) {
    if (!a.has_value()) return os << "null";
    if (a.type() == typeid(int)) return os << std::any_cast<int>(a);
    if (a.type() == typeid(int64_t)) return os << std::any_cast<int64_t>(a);
    if (a.type() == typeid(double)) return os << std::any_cast<double>(a);
    if (a.type() == typeid(float)) return os << std::any_cast<float>(a);
    if (a.type() == typeid(bool)) return os << (std::any_cast<bool>(a) ? "true" : "false");
    if (a.type() == typeid(char)) return os << std::any_cast<char>(a);
    if (a.type() == typeid(std::string)) return os << std::any_cast<std::string>(a);
    return os << "<?>";
}

// Stream output for std::vector (keys(), values() etc.)
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i > 0) os << ", ";
        os << v[i];
    }
    return os << "]";
}

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
