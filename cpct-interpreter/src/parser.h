// parser.h
// Parser declaration — converts a token array into an AST (Abstract Syntax Tree).
// Uses recursive descent for statements and precedence climbing for expressions.
#pragma once
#include "ast.h"
#include "token.h"
#include <vector>
#include <string>
#include <stdexcept>

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    // Parses the entire token array and returns a list of top-level statements (StmtPtr).
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_;

    // --- Utilities ---
    const Token& current() const;
    const Token& peek() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);       // check + advance (on match)
    Token expect(TokenType type, const std::string& msg); // error if expected token is missing
    [[noreturn]] void error(const std::string& msg);
    bool isTypeKeyword(TokenType type) const;
    std::string parseType();

    // --- Statement parsing ---
    StmtPtr parseStatement();         // dispatches to the appropriate parser based on current token
    StmtPtr parseVarDecl(const std::string& type);
    StmtPtr parseArrayDecl(const std::string& baseType, const std::string& name);
    StmtPtr parseDictDecl();          // dict name = {...};
    StmtPtr parseMapDecl();           // map<K,V> name = {...};
    StmtPtr parseVectorDecl();        // vector<T> name = [...];
    ExprPtr parseDictLiteral(int line);
    StmtPtr parseFuncDecl(const std::string& retType, const std::string& name);
    StmtPtr parseBlock();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseDoWhileStmt();
    StmtPtr parseForStmt();           // 4 for variants: forEach, for(n), chained-comparison for, C-style
    StmtPtr parseSwitchStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parsePrintStmt();
    StmtPtr parseExprStatement();

    // --- Expression parsing (low precedence → high precedence) ---
    ExprPtr parseExpression();        // = parseAssignment
    ExprPtr parseAssignment();        // =, +=, -=, ...
    ExprPtr parseTernary();           // ? :
    ExprPtr parseOr();                // ||
    ExprPtr parseAnd();               // &&
    ExprPtr parseBitOr();             // |
    ExprPtr parseBitXor();            // ^
    ExprPtr parseBitAnd();            // &
    ExprPtr parseEquality();          // ==, !=
    ExprPtr parseComparison();        // <, >, <=, >= (detects chained comparisons)
    ExprPtr parseShift();             // <<, >>
    ExprPtr parseAddition();          // +, -
    ExprPtr parseMultiplication();    // *, /, %, /%
    ExprPtr parsePower();             // ** (right-associative)
    ExprPtr parseUnary();             // -, !, ~, ++x, --x
    ExprPtr parsePostfix();           // x[], x.method(), x++, x--
    ExprPtr parsePrimary();           // literals, identifiers, function calls, parenthesized exprs, type casts
    ExprPtr parseFStringExpr();       // f"..." internal expressions via sub-lexer + sub-parser
};
