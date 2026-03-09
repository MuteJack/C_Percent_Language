#pragma once
#include <string>
#include <unordered_map>

enum class TokenType {
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT, CHAR_LIT, BOOL_LIT, FSTRING_LIT,

    // Identifier
    IDENTIFIER,

    // Types
    KW_INT, KW_INT8, KW_INT16, KW_INT32, KW_INT64, KW_INTBIG, KW_BIGINT,
    KW_UINT, KW_UINT8, KW_UINT16, KW_UINT32, KW_UINT64,
    KW_FLOAT, KW_FLOAT32, KW_FLOAT64,
    KW_CHAR, KW_STRING, KW_BOOL, KW_VOID,

    // Keywords
    KW_IF, KW_ELSE, KW_WHILE, KW_DO, KW_FOR, KW_RETURN,
    KW_PRINT, KW_PRINTLN, KW_INPUT, KW_TRUE, KW_FALSE,
    KW_BREAK, KW_CONTINUE, KW_SWITCH, KW_CASE, KW_DEFAULT,
    KW_DICT, KW_MAP, KW_VECTOR,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, POWER, DIVMOD,
    ASSIGN, EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN, PERCENT_ASSIGN,
    INCREMENT, DECREMENT,

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COMMA, COLON, DOT,

    // Special
    EOF_TOKEN, ERROR
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;

    Token() : type(TokenType::EOF_TOKEN), line(0), col(0) {}
    Token(TokenType type, std::string value, int line, int col)
        : type(type), value(std::move(value)), line(line), col(col) {}
};

inline const std::unordered_map<std::string, TokenType>& getKeywords() {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"int",      TokenType::KW_INT},
        {"int8",     TokenType::KW_INT8},
        {"int16",    TokenType::KW_INT16},
        {"int32",    TokenType::KW_INT32},
        {"int64",    TokenType::KW_INT64},
        {"intbig",   TokenType::KW_INTBIG},
        {"bigint",   TokenType::KW_BIGINT},
        {"uint",     TokenType::KW_UINT},
        {"uint8",    TokenType::KW_UINT8},
        {"uint16",   TokenType::KW_UINT16},
        {"uint32",   TokenType::KW_UINT32},
        {"uint64",   TokenType::KW_UINT64},
        {"float",    TokenType::KW_FLOAT},
        {"float32",  TokenType::KW_FLOAT32},
        {"float64",  TokenType::KW_FLOAT64},
        {"char",     TokenType::KW_CHAR},
        {"string",   TokenType::KW_STRING},
        {"bool",     TokenType::KW_BOOL},
        {"void",     TokenType::KW_VOID},
        {"if",       TokenType::KW_IF},
        {"else",     TokenType::KW_ELSE},
        {"while",    TokenType::KW_WHILE},
        {"do",       TokenType::KW_DO},
        {"for",      TokenType::KW_FOR},
        {"return",   TokenType::KW_RETURN},
        {"print",    TokenType::KW_PRINT},
        {"println",  TokenType::KW_PRINTLN},
        {"input",    TokenType::KW_INPUT},
        {"true",     TokenType::KW_TRUE},
        {"false",    TokenType::KW_FALSE},
        {"break",    TokenType::KW_BREAK},
        {"continue", TokenType::KW_CONTINUE},
        {"switch",   TokenType::KW_SWITCH},
        {"case",     TokenType::KW_CASE},
        {"default",  TokenType::KW_DEFAULT},
        {"dict",     TokenType::KW_DICT},
        {"map",      TokenType::KW_MAP},
        {"vector",   TokenType::KW_VECTOR},
    };
    return keywords;
}

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INT_LIT: return "INT_LIT";
        case TokenType::FLOAT_LIT: return "FLOAT_LIT";
        case TokenType::STRING_LIT: return "STRING_LIT";
        case TokenType::CHAR_LIT: return "CHAR_LIT";
        case TokenType::BOOL_LIT: return "BOOL_LIT";
        case TokenType::FSTRING_LIT: return "FSTRING_LIT";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::KW_INT: return "int";
        case TokenType::KW_INT8: return "int8";
        case TokenType::KW_INT16: return "int16";
        case TokenType::KW_INT32: return "int32";
        case TokenType::KW_INT64: return "int64";
        case TokenType::KW_INTBIG: return "intbig";
        case TokenType::KW_BIGINT: return "bigint";
        case TokenType::KW_UINT: return "uint";
        case TokenType::KW_UINT8: return "uint8";
        case TokenType::KW_UINT16: return "uint16";
        case TokenType::KW_UINT32: return "uint32";
        case TokenType::KW_UINT64: return "uint64";
        case TokenType::KW_FLOAT: return "float";
        case TokenType::KW_FLOAT32: return "float32";
        case TokenType::KW_FLOAT64: return "float64";
        case TokenType::KW_CHAR: return "char";
        case TokenType::KW_STRING: return "string";
        case TokenType::KW_BOOL: return "bool";
        case TokenType::KW_VOID: return "void";
        case TokenType::KW_IF: return "if";
        case TokenType::KW_ELSE: return "else";
        case TokenType::KW_WHILE: return "while";
        case TokenType::KW_DO: return "do";
        case TokenType::KW_FOR: return "for";
        case TokenType::KW_RETURN: return "return";
        case TokenType::KW_PRINT: return "print";
        case TokenType::KW_PRINTLN: return "println";
        case TokenType::KW_INPUT: return "input";
        case TokenType::KW_TRUE: return "true";
        case TokenType::KW_FALSE: return "false";
        case TokenType::KW_BREAK: return "break";
        case TokenType::KW_CONTINUE: return "continue";
        case TokenType::KW_SWITCH: return "switch";
        case TokenType::KW_CASE: return "case";
        case TokenType::KW_DEFAULT: return "default";
        case TokenType::KW_DICT: return "dict";
        case TokenType::KW_MAP: return "map";
        case TokenType::KW_VECTOR: return "vector";
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::PERCENT: return "%";
        case TokenType::POWER: return "**";
        case TokenType::DIVMOD: return "/%";
        case TokenType::ASSIGN: return "=";
        case TokenType::EQ: return "==";
        case TokenType::NEQ: return "!=";
        case TokenType::LT: return "<";
        case TokenType::GT: return ">";
        case TokenType::LTE: return "<=";
        case TokenType::GTE: return ">=";
        case TokenType::AND: return "&&";
        case TokenType::OR: return "||";
        case TokenType::NOT: return "!";
        case TokenType::PLUS_ASSIGN: return "+=";
        case TokenType::MINUS_ASSIGN: return "-=";
        case TokenType::STAR_ASSIGN: return "*=";
        case TokenType::SLASH_ASSIGN: return "/=";
        case TokenType::PERCENT_ASSIGN: return "%=";
        case TokenType::INCREMENT: return "++";
        case TokenType::DECREMENT: return "--";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::LBRACKET: return "[";
        case TokenType::RBRACKET: return "]";
        case TokenType::SEMICOLON: return ";";
        case TokenType::COMMA: return ",";
        case TokenType::COLON: return ":";
        case TokenType::DOT: return ".";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}
