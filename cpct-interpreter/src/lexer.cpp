// lexer.cpp
// Lexer implementation.
// tokenize() is the main loop, dispatching to make* functions by character type.
// Supported literals: integer, float, string(""), raw string(r""), f-string(f""), char('')
// Operators are disambiguated by peeking 1-2 characters ahead.
#include "lexer.h"
#include <stdexcept>

Lexer::Lexer(const std::string& source)
    : source_(source), pos_(0), line_(1), col_(1) {}

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::peek() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

void Lexer::advance() {
    if (current() == '\n') {
        line_++;
        col_ = 0;
    }
    pos_++;
    col_++;
}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(current())) {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (pos_ < source_.size() && current() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    advance(); // skip /
    advance(); // skip *
    while (pos_ < source_.size()) {
        if (current() == '*' && peek() == '/') {
            advance(); // skip *
            advance(); // skip /
            return;
        }
        advance();
    }
    throw std::runtime_error("Unterminated block comment at line " + std::to_string(line_));
}

Token Lexer::makeNumber() {
    int startCol = col_;
    std::string num;
    bool isFloat = false;

    while (pos_ < source_.size() && (std::isdigit(current()) || current() == '.')) {
        if (current() == '.') {
            if (isFloat) break; // second dot, stop
            isFloat = true;
        }
        num += current();
        advance();
    }

    return Token(isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, num, line_, startCol);
}

Token Lexer::makeString() {
    int startCol = col_;
    advance(); // skip opening quote
    std::string str;

    while (pos_ < source_.size() && current() != '"') {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '\\': str += '\\'; break;
                case '"': str += '"'; break;
                case '0': str += '\0'; break;
                default: str += current(); break;
            }
        } else {
            str += current();
        }
        advance();
    }

    if (pos_ >= source_.size()) {
        throw std::runtime_error("Unterminated string at line " + std::to_string(line_));
    }
    advance(); // skip closing quote

    return Token(TokenType::STRING_LIT, str, line_, startCol);
}

Token Lexer::makeRawString() {
    int startCol = col_;
    advance(); // skip 'r'
    advance(); // skip opening quote
    std::string str;

    while (pos_ < source_.size() && current() != '"') {
        str += current();
        advance();
    }

    if (pos_ >= source_.size()) {
        throw std::runtime_error("Unterminated raw string at line " + std::to_string(line_));
    }
    advance(); // skip closing quote

    return Token(TokenType::STRING_LIT, str, line_, startCol);
}

Token Lexer::makeFString() {
    int startCol = col_;
    advance(); // skip 'f'
    advance(); // skip opening quote
    std::string raw;
    int braceDepth = 0;

    while (pos_ < source_.size()) {
        if (current() == '"' && braceDepth == 0) break;

        if (current() == '\\' && braceDepth == 0) {
            raw += current();
            advance();
            if (pos_ < source_.size()) {
                raw += current();
                advance();
            }
            continue;
        }

        if (current() == '{') braceDepth++;
        else if (current() == '}') {
            if (braceDepth > 0) braceDepth--;
        }

        raw += current();
        advance();
    }

    if (pos_ >= source_.size()) {
        throw std::runtime_error("Unterminated f-string at line " + std::to_string(line_));
    }
    advance(); // skip closing quote

    return Token(TokenType::FSTRING_LIT, raw, line_, startCol);
}

Token Lexer::makeChar() {
    int startCol = col_;
    advance(); // skip opening single quote
    std::string ch;

    if (pos_ >= source_.size() || current() == '\'') {
        throw std::runtime_error("Empty char literal at line " + std::to_string(line_));
    }

    if (current() == '\\') {
        advance();
        switch (current()) {
            case 'n': ch += '\n'; break;
            case 't': ch += '\t'; break;
            case '\\': ch += '\\'; break;
            case '\'': ch += '\''; break;
            case '0': ch += '\0'; break;
            case 'r': ch += '\r'; break;
            default: ch += current(); break;
        }
    } else {
        ch += current();
    }
    advance();

    if (pos_ >= source_.size() || current() != '\'') {
        throw std::runtime_error("Unterminated or multi-character char literal at line " + std::to_string(line_));
    }
    advance(); // skip closing single quote

    return Token(TokenType::CHAR_LIT, ch, line_, startCol);
}

Token Lexer::makeIdentifierOrKeyword() {
    int startCol = col_;
    std::string id;

    while (pos_ < source_.size() && (std::isalnum(current()) || current() == '_')) {
        id += current();
        advance();
    }

    auto& keywords = getKeywords();
    auto it = keywords.find(id);
    if (it != keywords.end()) {
        if (it->second == TokenType::KW_TRUE || it->second == TokenType::KW_FALSE) {
            return Token(TokenType::BOOL_LIT, id, line_, startCol);
        }
        return Token(it->second, id, line_, startCol);
    }

    return Token(TokenType::IDENTIFIER, id, line_, startCol);
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    int startCol = col_;
    for (size_t i = 0; i < value.size(); i++) advance();
    return Token(type, value, line_, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos_ < source_.size()) {
        skipWhitespace();
        if (pos_ >= source_.size()) break;

        char c = current();

        // Comments
        if (c == '/' && peek() == '/') {
            skipLineComment();
            continue;
        }
        if (c == '/' && peek() == '*') {
            skipBlockComment();
            continue;
        }

        // Numbers
        if (std::isdigit(c)) {
            tokens.push_back(makeNumber());
            continue;
        }

        // Strings
        if (c == '"') {
            tokens.push_back(makeString());
            continue;
        }

        // Char literals
        if (c == '\'') {
            tokens.push_back(makeChar());
            continue;
        }

        // Raw string r"..." and f-string f"..."
        if ((c == 'r' || c == 'f') && peek() == '"') {
            if (c == 'r') {
                tokens.push_back(makeRawString());
            } else {
                tokens.push_back(makeFString());
            }
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(makeIdentifierOrKeyword());
            continue;
        }

        // Two-character operators
        switch (c) {
            case '+':
                if (peek() == '+') { tokens.push_back(makeToken(TokenType::INCREMENT, "++")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::PLUS_ASSIGN, "+=")); continue; }
                tokens.push_back(makeToken(TokenType::PLUS, "+"));
                continue;
            case '-':
                if (peek() == '-') { tokens.push_back(makeToken(TokenType::DECREMENT, "--")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::MINUS_ASSIGN, "-=")); continue; }
                tokens.push_back(makeToken(TokenType::MINUS, "-"));
                continue;
            case '*':
                if (peek() == '*') { tokens.push_back(makeToken(TokenType::POWER, "**")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::STAR_ASSIGN, "*=")); continue; }
                tokens.push_back(makeToken(TokenType::STAR, "*"));
                continue;
            case '/':
                if (peek() == '%') { tokens.push_back(makeToken(TokenType::DIVMOD, "/%")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::SLASH_ASSIGN, "/=")); continue; }
                tokens.push_back(makeToken(TokenType::SLASH, "/"));
                continue;
            case '%':
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::PERCENT_ASSIGN, "%=")); continue; }
                tokens.push_back(makeToken(TokenType::PERCENT, "%"));
                continue;
            case '=':
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::EQ, "==")); continue; }
                tokens.push_back(makeToken(TokenType::ASSIGN, "="));
                continue;
            case '!':
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::NEQ, "!=")); continue; }
                tokens.push_back(makeToken(TokenType::NOT, "!"));
                continue;
            case '<':
                if (peek() == '<') {
                    if (pos_ + 2 < source_.size() && source_[pos_ + 2] == '=') {
                        tokens.push_back(makeToken(TokenType::LSHIFT_ASSIGN, "<<=")); continue;
                    }
                    tokens.push_back(makeToken(TokenType::LSHIFT, "<<")); continue;
                }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::LTE, "<=")); continue; }
                tokens.push_back(makeToken(TokenType::LT, "<"));
                continue;
            case '>':
                if (peek() == '>') {
                    if (pos_ + 2 < source_.size() && source_[pos_ + 2] == '=') {
                        tokens.push_back(makeToken(TokenType::RSHIFT_ASSIGN, ">>=")); continue;
                    }
                    tokens.push_back(makeToken(TokenType::RSHIFT, ">>")); continue;
                }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::GTE, ">=")); continue; }
                tokens.push_back(makeToken(TokenType::GT, ">"));
                continue;
            case '&':
                if (peek() == '&') { tokens.push_back(makeToken(TokenType::AND, "&&")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::BIT_AND_ASSIGN, "&=")); continue; }
                tokens.push_back(makeToken(TokenType::BIT_AND, "&"));
                continue;
            case '|':
                if (peek() == '|') { tokens.push_back(makeToken(TokenType::OR, "||")); continue; }
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::BIT_OR_ASSIGN, "|=")); continue; }
                tokens.push_back(makeToken(TokenType::BIT_OR, "|"));
                continue;
            case '^':
                if (peek() == '=') { tokens.push_back(makeToken(TokenType::BIT_XOR_ASSIGN, "^=")); continue; }
                tokens.push_back(makeToken(TokenType::BIT_XOR, "^"));
                continue;
            case '~':
                tokens.push_back(makeToken(TokenType::BIT_NOT, "~"));
                continue;
            case '(':
                tokens.push_back(makeToken(TokenType::LPAREN, "("));
                continue;
            case ')':
                tokens.push_back(makeToken(TokenType::RPAREN, ")"));
                continue;
            case '{':
                tokens.push_back(makeToken(TokenType::LBRACE, "{"));
                continue;
            case '}':
                tokens.push_back(makeToken(TokenType::RBRACE, "}"));
                continue;
            case '[':
                tokens.push_back(makeToken(TokenType::LBRACKET, "["));
                continue;
            case ']':
                tokens.push_back(makeToken(TokenType::RBRACKET, "]"));
                continue;
            case ';':
                tokens.push_back(makeToken(TokenType::SEMICOLON, ";"));
                continue;
            case ',':
                tokens.push_back(makeToken(TokenType::COMMA, ","));
                continue;
            case ':':
                tokens.push_back(makeToken(TokenType::COLON, ":"));
                continue;
            case '.':
                tokens.push_back(makeToken(TokenType::DOT, "."));
                continue;
            case '?':
                tokens.push_back(makeToken(TokenType::QUESTION, "?"));
                continue;
        }

        throw std::runtime_error(
            "Unexpected character '" + std::string(1, c) +
            "' at line " + std::to_string(line_) + ", col " + std::to_string(col_));
    }

    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line_, col_));
    return tokens;
}
