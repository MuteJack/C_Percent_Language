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
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_;

    // Utilities
    const Token& current() const;
    const Token& peek() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& msg);
    [[noreturn]] void error(const std::string& msg);
    bool isTypeKeyword(TokenType type) const;
    std::string parseType();

    // Statements
    StmtPtr parseStatement();
    StmtPtr parseVarDecl(const std::string& type);
    StmtPtr parseArrayDecl(const std::string& baseType, const std::string& name);
    StmtPtr parseDictDecl();
    StmtPtr parseMapDecl();
    StmtPtr parseVectorDecl();
    ExprPtr parseDictLiteral(int line);
    StmtPtr parseFuncDecl(const std::string& retType, const std::string& name);
    StmtPtr parseBlock();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseDoWhileStmt();
    StmtPtr parseForStmt();
    StmtPtr parseSwitchStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parsePrintStmt();
    StmtPtr parseExprStatement();

    // Expressions (precedence climbing)
    ExprPtr parseExpression();
    ExprPtr parseAssignment();
    ExprPtr parseTernary();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiplication();
    ExprPtr parsePower();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parseFStringExpr();
};
