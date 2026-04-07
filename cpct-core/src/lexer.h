// lexer.h
// Lexer declaration — converts a source string into a token array.
#pragma once
#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    // Scans the entire source and returns a token vector (appends EOF_TOKEN at the end).
    std::vector<Token> tokenize();

private:
    std::string source_;  // entire source code
    size_t pos_;          // current scan position
    int line_;            // current line number (1-based)
    int col_;             // current column number (1-based)

    char current() const;   // current character ('\0' at EOF)
    char peek() const;      // lookahead next character
    void advance();          // advance pos_, update line_/col_ on newline
    void skipWhitespace();
    void skipLineComment();  // // comments
    void skipBlockComment(); // /* */ comments
    Token makeNumber();      // produces integer/float literal tokens
    Token makeString();      // "..." string (with escape processing)
    Token makeRawString();   // r"..." raw string (no escape processing)
    Token makeFString();     // f"..." format string (tracks brace depth)
    Token makeChar();        // '.' character literal
    Token makeIdentifierOrKeyword(); // identifier or reserved keyword
    Token makeToken(TokenType type, const std::string& value); // helper for N-character operators
};
