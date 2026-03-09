#pragma once
#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_;
    int line_;
    int col_;

    char current() const;
    char peek() const;
    void advance();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token makeNumber();
    Token makeString();
    Token makeRawString();
    Token makeFString();
    Token makeChar();
    Token makeIdentifierOrKeyword();
    Token makeToken(TokenType type, const std::string& value);
};
