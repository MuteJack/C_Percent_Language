// preprocessor.h
// Preprocessor — handles #define and #undef directives before lexing.
// Performs text substitution on the source string, supporting:
//   - Simple constants:   #define NAME value
//   - Macro functions:    #define MAX(a, b) ((a) > (b) ? (a) : (b))
//   - Undefine:           #undef NAME
//   - Nested macro expansion (macros referencing other macros)
// String literals ("..." and '...') are not subject to substitution.
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

class PreprocessorError : public std::runtime_error {
public:
    PreprocessorError(const std::string& msg, int line)
        : std::runtime_error("Preprocessor error at line " + std::to_string(line) + ": " + msg),
          line_(line) {}
    int line() const { return line_; }
private:
    int line_;
};

struct MacroDef {
    std::string name;
    std::vector<std::string> params; // empty for simple constants
    std::string body;
    bool isFunction = false; // true if macro has parameter list
};

class Preprocessor {
public:
    // Processes the source, expanding #define/#undef directives and returning the result.
    std::string process(const std::string& source) {
        defines_.clear();
        std::istringstream stream(source);
        std::string result;
        std::string line;
        int lineNum = 0;

        while (std::getline(stream, line)) {
            lineNum++;
            std::string trimmed = trim(line);

            if (trimmed.rfind("#define", 0) == 0) {
                parseDefine(trimmed, lineNum);
                result += "\n"; // preserve line numbers
            } else if (trimmed.rfind("#undef", 0) == 0) {
                parseUndef(trimmed, lineNum);
                result += "\n";
            } else {
                result += expandLine(line, lineNum) + "\n";
            }
        }

        // Remove trailing newline added by loop
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }

private:
    std::unordered_map<std::string, MacroDef> defines_;

    // Parses a #define directive and registers the macro.
    void parseDefine(const std::string& line, int lineNum) {
        // Skip "#define"
        size_t pos = 7;
        skipSpaces(line, pos);

        if (pos >= line.size() || !isIdentStart(line[pos])) {
            throw PreprocessorError("Expected macro name after #define", lineNum);
        }

        // Read macro name
        std::string name = readIdent(line, pos);

        MacroDef macro;
        macro.name = name;

        // Check for function-like macro: NAME( immediately after name, no space
        if (pos < line.size() && line[pos] == '(') {
            macro.isFunction = true;
            pos++; // skip '('
            // Read parameter list
            while (pos < line.size() && line[pos] != ')') {
                skipSpaces(line, pos);
                if (pos < line.size() && line[pos] == ')') break;
                if (!isIdentStart(line[pos])) {
                    throw PreprocessorError("Expected parameter name in macro '" + name + "'", lineNum);
                }
                macro.params.push_back(readIdent(line, pos));
                skipSpaces(line, pos);
                if (pos < line.size() && line[pos] == ',') pos++;
            }
            if (pos >= line.size() || line[pos] != ')') {
                throw PreprocessorError("Missing ')' in macro '" + name + "'", lineNum);
            }
            pos++; // skip ')'
        }

        // Rest is the body
        skipSpaces(line, pos);
        macro.body = (pos < line.size()) ? line.substr(pos) : "";

        defines_[name] = macro;
    }

    // Parses a #undef directive and removes the macro.
    void parseUndef(const std::string& line, int lineNum) {
        size_t pos = 6; // skip "#undef"
        skipSpaces(line, pos);

        if (pos >= line.size() || !isIdentStart(line[pos])) {
            throw PreprocessorError("Expected macro name after #undef", lineNum);
        }

        std::string name = readIdent(line, pos);
        defines_.erase(name);
    }

    // Expands all macros in a single line, respecting string literals.
    std::string expandLine(const std::string& line, int lineNum) {
        std::string result = line;
        // Repeat expansion for nested macros (max depth to prevent infinite recursion)
        const int maxDepth = 32;
        for (int depth = 0; depth < maxDepth; depth++) {
            std::string expanded = expandOnce(result, lineNum);
            if (expanded == result) break; // no more expansions
            result = expanded;
        }
        return result;
    }

    // Single pass of macro expansion over a line.
    std::string expandOnce(const std::string& line, int lineNum) {
        std::string result;
        size_t i = 0;

        while (i < line.size()) {
            // Skip string literals
            if (line[i] == '"') {
                result += readStringLiteral(line, i);
                continue;
            }
            if (line[i] == '\'') {
                result += readCharLiteral(line, i);
                continue;
            }

            // Check for f-string: f"..."
            if (line[i] == 'f' && i + 1 < line.size() && line[i + 1] == '"') {
                // Check if 'f' is part of a longer identifier
                if (i > 0 && isIdentChar(line[i - 1])) {
                    result += line[i++];
                    continue;
                }
                result += readFStringLiteral(line, i, lineNum);
                continue;
            }

            // Check for r-string: r"..."
            if (line[i] == 'r' && i + 1 < line.size() && line[i + 1] == '"') {
                if (i > 0 && isIdentChar(line[i - 1])) {
                    result += line[i++];
                    continue;
                }
                result += readRawStringLiteral(line, i);
                continue;
            }

            // Try to read an identifier
            if (isIdentStart(line[i])) {
                std::string ident = readIdent(line, i);

                auto it = defines_.find(ident);
                if (it == defines_.end()) {
                    result += ident;
                    continue;
                }

                const MacroDef& macro = it->second;

                if (macro.isFunction) {
                    // Need '(' after name
                    size_t savedI = i;
                    skipSpaces(line, i);
                    if (i < line.size() && line[i] == '(') {
                        i++; // skip '('
                        auto args = readMacroArgs(line, i, lineNum);
                        if (args.size() != macro.params.size()) {
                            throw PreprocessorError(
                                "Macro '" + macro.name + "' expects " +
                                std::to_string(macro.params.size()) + " arguments, got " +
                                std::to_string(args.size()), lineNum);
                        }
                        result += substituteParams(macro.body, macro.params, args);
                    } else {
                        // No '(' — not a macro invocation, output as-is
                        i = savedI;
                        result += ident;
                    }
                } else {
                    result += macro.body;
                }
                continue;
            }

            result += line[i++];
        }

        return result;
    }

    // Reads arguments for a function-like macro call, handling nested parens.
    std::vector<std::string> readMacroArgs(const std::string& line, size_t& pos, int lineNum) {
        std::vector<std::string> args;
        int depth = 1;
        std::string current;

        while (pos < line.size() && depth > 0) {
            char c = line[pos];
            if (c == '(') {
                depth++;
                current += c;
            } else if (c == ')') {
                depth--;
                if (depth == 0) {
                    args.push_back(trim(current));
                } else {
                    current += c;
                }
            } else if (c == ',' && depth == 1) {
                args.push_back(trim(current));
                current.clear();
            } else if (c == '"') {
                current += readStringLiteral(line, pos);
                continue;
            } else if (c == '\'') {
                current += readCharLiteral(line, pos);
                continue;
            } else {
                current += c;
            }
            pos++;
        }

        if (depth != 0) {
            throw PreprocessorError("Unterminated macro argument list", lineNum);
        }

        // Handle empty argument list: #define FOO() → args should be empty
        if (args.size() == 1 && args[0].empty()) {
            args.clear();
        }

        return args;
    }

    // Substitutes macro parameters in the body with actual arguments.
    std::string substituteParams(const std::string& body,
                                 const std::vector<std::string>& params,
                                 const std::vector<std::string>& args) {
        std::string result;
        size_t i = 0;

        while (i < body.size()) {
            if (body[i] == '"') {
                result += readStringLiteral(body, i);
                continue;
            }
            if (body[i] == '\'') {
                result += readCharLiteral(body, i);
                continue;
            }
            if (isIdentStart(body[i])) {
                std::string ident = readIdent(body, i);
                bool found = false;
                for (size_t p = 0; p < params.size(); p++) {
                    if (ident == params[p]) {
                        result += args[p];
                        found = true;
                        break;
                    }
                }
                if (!found) result += ident;
                continue;
            }
            result += body[i++];
        }

        return result;
    }

    // Reads a "..." string literal (with escape sequences), advancing pos past the closing quote.
    static std::string readStringLiteral(const std::string& s, size_t& pos) {
        std::string result;
        result += s[pos++]; // opening "
        while (pos < s.size()) {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                result += s[pos++];
                result += s[pos++];
            } else if (s[pos] == '"') {
                result += s[pos++]; // closing "
                return result;
            } else {
                result += s[pos++];
            }
        }
        return result; // unterminated — lexer will catch it
    }

    // Reads a '.' character literal, advancing pos past the closing quote.
    static std::string readCharLiteral(const std::string& s, size_t& pos) {
        std::string result;
        result += s[pos++]; // opening '
        while (pos < s.size()) {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                result += s[pos++];
                result += s[pos++];
            } else if (s[pos] == '\'') {
                result += s[pos++]; // closing '
                return result;
            } else {
                result += s[pos++];
            }
        }
        return result;
    }

    // Reads an f"..." format string literal, expanding macros inside {expr} blocks.
    std::string readFStringLiteral(const std::string& s, size_t& pos, int lineNum) {
        std::string result;
        result += s[pos++]; // 'f'
        result += s[pos++]; // opening "
        while (pos < s.size()) {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                result += s[pos++];
                result += s[pos++];
            } else if (s[pos] == '{') {
                result += s[pos++]; // '{'
                // Read expression inside braces, tracking depth
                int depth = 1;
                std::string expr;
                while (pos < s.size() && depth > 0) {
                    if (s[pos] == '{') { depth++; expr += s[pos++]; }
                    else if (s[pos] == '}') {
                        depth--;
                        if (depth == 0) { pos++; break; }
                        expr += s[pos++];
                    } else if (s[pos] == '"') {
                        expr += readStringLiteral(s, pos);
                    } else {
                        expr += s[pos++];
                    }
                }
                // Expand macros in the expression
                std::string expanded = expandOnce(expr, lineNum);
                result += expanded + "}";
            } else if (s[pos] == '"') {
                result += s[pos++]; // closing "
                return result;
            } else {
                result += s[pos++];
            }
        }
        return result;
    }

    // Reads an r"..." raw string literal.
    static std::string readRawStringLiteral(const std::string& s, size_t& pos) {
        std::string result;
        result += s[pos++]; // 'r'
        result += s[pos++]; // opening "
        while (pos < s.size()) {
            if (s[pos] == '"') {
                result += s[pos++]; // closing "
                return result;
            }
            result += s[pos++];
        }
        return result;
    }

    // Reads an identifier starting at pos, advancing pos past the end.
    static std::string readIdent(const std::string& s, size_t& pos) {
        std::string result;
        while (pos < s.size() && isIdentChar(s[pos])) {
            result += s[pos++];
        }
        return result;
    }

    static bool isIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    static bool isIdentChar(char c) {
        return isIdentStart(c) || (c >= '0' && c <= '9');
    }

    static void skipSpaces(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) pos++;
    }

    static std::string trim(const std::string& s) {
        size_t start = 0, end = s.size();
        while (start < end && (s[start] == ' ' || s[start] == '\t')) start++;
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) end--;
        return s.substr(start, end - start);
    }
};
