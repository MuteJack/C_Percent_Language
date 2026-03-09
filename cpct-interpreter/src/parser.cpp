#include "parser.h"
#include "lexer.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

const Token& Parser::current() const { return tokens_[pos_]; }

const Token& Parser::peek() const {
    if (pos_ + 1 < tokens_.size()) return tokens_[pos_ + 1];
    return tokens_.back();
}

const Token& Parser::advance() {
    const Token& tok = tokens_[pos_];
    if (pos_ < tokens_.size() - 1) pos_++;
    return tok;
}

bool Parser::check(TokenType type) const { return current().type == type; }

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

Token Parser::expect(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    error(msg + " (got '" + current().value + "' at line " + std::to_string(current().line) + ")");
}

void Parser::error(const std::string& msg) {
    throw ParseError("Parse error: " + msg);
}

bool Parser::isTypeKeyword(TokenType type) const {
    return type == TokenType::KW_INT || type == TokenType::KW_INT8 ||
           type == TokenType::KW_INT16 || type == TokenType::KW_INT32 ||
           type == TokenType::KW_INT64 || type == TokenType::KW_INTBIG ||
           type == TokenType::KW_BIGINT ||
           type == TokenType::KW_UINT || type == TokenType::KW_UINT8 ||
           type == TokenType::KW_UINT16 || type == TokenType::KW_UINT32 ||
           type == TokenType::KW_UINT64 ||
           type == TokenType::KW_FLOAT ||
           type == TokenType::KW_FLOAT32 || type == TokenType::KW_FLOAT64 ||
           type == TokenType::KW_CHAR ||
           type == TokenType::KW_STRING || type == TokenType::KW_BOOL ||
           type == TokenType::KW_VOID;
}

std::string Parser::parseType() {
    Token tok = advance();
    return tok.value;
}

// ============== Parsing ==============

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> program;
    while (!check(TokenType::EOF_TOKEN)) {
        program.push_back(parseStatement());
    }
    return program;
}

StmtPtr Parser::parseStatement() {
    // Type keyword -> could be variable decl, function decl, or type cast
    if (isTypeKeyword(current().type)) {
        // Type cast: int(expr) or type method: int.max()
        if (peek().type == TokenType::LPAREN || peek().type == TokenType::DOT) {
            return parseExprStatement();
        }

        std::string type = parseType();
        std::string name = expect(TokenType::IDENTIFIER, "Expected identifier after type").value;

        // Function declaration
        if (check(TokenType::LPAREN)) {
            return parseFuncDecl(type, name);
        }

        // Array declaration: type name[...][...] ...
        if (check(TokenType::LBRACKET)) {
            return parseArrayDecl(type, name);
        }

        // Variable declaration
        return parseVarDecl(type);
    }

    // Dict declaration: dict name = {...};  (untyped, Python-style)
    if (check(TokenType::KW_DICT)) {
        return parseDictDecl();
    }

    // Map declaration: map<KeyType, ValueType> name = {...};  (typed, C++ style)
    if (check(TokenType::KW_MAP)) {
        return parseMapDecl();
    }

    // Vector declaration: vector<Type> name = [...];  (typed dynamic array)
    if (check(TokenType::KW_VECTOR)) {
        return parseVectorDecl();
    }

    if (check(TokenType::KW_IF)) return parseIfStmt();
    if (check(TokenType::KW_WHILE)) return parseWhileStmt();
    if (check(TokenType::KW_DO)) return parseDoWhileStmt();
    if (check(TokenType::KW_FOR)) return parseForStmt();
    if (check(TokenType::KW_SWITCH)) return parseSwitchStmt();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::KW_PRINT) || check(TokenType::KW_PRINTLN)) return parsePrintStmt();
    if (check(TokenType::LBRACE)) return parseBlock();

    if (check(TokenType::KW_BREAK)) {
        int line = current().line;
        advance();
        expect(TokenType::SEMICOLON, "Expected ';' after break");
        return makeBreak(line);
    }
    if (check(TokenType::KW_CONTINUE)) {
        int line = current().line;
        advance();
        expect(TokenType::SEMICOLON, "Expected ';' after continue");
        return makeContinue(line);
    }

    return parseExprStatement();
}

StmtPtr Parser::parseVarDecl(const std::string& type) {
    // name already consumed, we need to handle: name; or name = expr;
    // Actually, name was consumed by parseStatement, let's adjust
    std::string name = tokens_[pos_ - 1].value;
    int line = tokens_[pos_ - 1].line;

    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return makeVarDecl(type, name, std::move(init), line);
}

StmtPtr Parser::parseArrayDecl(const std::string& baseType, const std::string& name) {
    int line = tokens_[pos_ - 1].line; // name token line
    std::string type = baseType;
    std::vector<ExprPtr> dimSizes;

    // Parse dimension brackets: [expr] or []
    while (check(TokenType::LBRACKET)) {
        advance(); // skip [
        type += "[]";
        if (check(TokenType::RBRACKET)) {
            // Empty brackets — size inferred from initializer
            dimSizes.push_back(nullptr);
        } else {
            // Explicit size expression
            dimSizes.push_back(parseExpression());
        }
        expect(TokenType::RBRACKET, "Expected ']' in array declaration");
    }

    // Optional initializer
    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after array declaration");

    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::VarDecl;
    s->varType = type;
    s->varName = name;
    s->initExpr = std::move(init);
    s->arrayDimSizes = std::move(dimSizes);
    s->line = line;
    return s;
}

StmtPtr Parser::parseDictDecl() {
    int line = current().line;
    advance(); // consume 'dict'

    // Variable name
    std::string name = expect(TokenType::IDENTIFIER, "Expected dict variable name").value;

    // Untyped dict — type string is just "dict"
    std::string type = "dict";

    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        // Parse dict literal: {key: value, ...}
        init = parseDictLiteral(line);
    }

    expect(TokenType::SEMICOLON, "Expected ';' after dict declaration");
    return makeVarDecl(type, name, std::move(init), line);
}

StmtPtr Parser::parseMapDecl() {
    int line = current().line;
    advance(); // consume 'map'

    // Parse <KeyType, ValueType>
    expect(TokenType::LT, "Expected '<' after 'map'");
    if (!isTypeKeyword(current().type)) {
        error("Expected type keyword for map key type, got '" + current().value + "'");
    }
    std::string keyType = advance().value;
    expect(TokenType::COMMA, "Expected ',' between key and value types");
    if (!isTypeKeyword(current().type)) {
        error("Expected type keyword for map value type, got '" + current().value + "'");
    }
    std::string valueType = advance().value;
    expect(TokenType::GT, "Expected '>' after value type");

    // Variable name
    std::string name = expect(TokenType::IDENTIFIER, "Expected map variable name").value;

    // Construct type string: "map<string,int>"
    std::string type = "map<" + keyType + "," + valueType + ">";

    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseDictLiteral(line);
    }

    expect(TokenType::SEMICOLON, "Expected ';' after map declaration");
    return makeVarDecl(type, name, std::move(init), line);
}

StmtPtr Parser::parseVectorDecl() {
    int line = current().line;
    advance(); // consume 'vector'

    // Parse <ElemType>
    expect(TokenType::LT, "Expected '<' after 'vector'");
    if (!isTypeKeyword(current().type)) {
        error("Expected type keyword for vector element type, got '" + current().value + "'");
    }
    std::string elemType = advance().value;
    expect(TokenType::GT, "Expected '>' after element type");

    // Variable name
    std::string name = expect(TokenType::IDENTIFIER, "Expected vector variable name").value;

    // Construct type string: "vector<int>"
    std::string type = "vector<" + elemType + ">";

    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        // Parse array literal: [val1, val2, ...]
        init = parsePrimary(); // parsePrimary handles [...] array literals
    }

    expect(TokenType::SEMICOLON, "Expected ';' after vector declaration");
    return makeVarDecl(type, name, std::move(init), line);
}

ExprPtr Parser::parseDictLiteral(int line) {
    expect(TokenType::LBRACE, "Expected '{' for dict literal");
    std::vector<ExprPtr> keys;
    std::vector<ExprPtr> values;

    if (!check(TokenType::RBRACE)) {
        // Parse first key: value pair
        keys.push_back(parseExpression());
        expect(TokenType::COLON, "Expected ':' after dict key");
        values.push_back(parseExpression());

        while (match(TokenType::COMMA)) {
            if (check(TokenType::RBRACE)) break; // trailing comma
            keys.push_back(parseExpression());
            expect(TokenType::COLON, "Expected ':' after dict key");
            values.push_back(parseExpression());
        }
    }

    expect(TokenType::RBRACE, "Expected '}' after dict literal");
    return makeDictLiteral(std::move(keys), std::move(values), line);
}

StmtPtr Parser::parseFuncDecl(const std::string& retType, const std::string& name) {
    int line = current().line;
    expect(TokenType::LPAREN, "Expected '(' after function name");

    std::vector<FuncParam> params;
    if (!check(TokenType::RPAREN)) {
        do {
            std::string paramType = parseType();
            std::string paramName = expect(TokenType::IDENTIFIER, "Expected parameter name").value;
            params.push_back({paramType, paramName});
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");

    expect(TokenType::LBRACE, "Expected '{' for function body");
    std::vector<StmtPtr> body;
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        body.push_back(parseStatement());
    }
    expect(TokenType::RBRACE, "Expected '}' after function body");

    return makeFuncDecl(retType, name, std::move(params), std::move(body), line);
}

StmtPtr Parser::parseBlock() {
    int line = current().line;
    expect(TokenType::LBRACE, "Expected '{'");

    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        stmts.push_back(parseStatement());
    }
    expect(TokenType::RBRACE, "Expected '}'");

    return makeBlock(std::move(stmts), line);
}

StmtPtr Parser::parseIfStmt() {
    int line = current().line;
    advance(); // skip 'if'
    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after if condition");

    auto thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        elseBranch = parseStatement();
    }

    return makeIf(std::move(cond), std::move(thenBranch), std::move(elseBranch), line);
}

StmtPtr Parser::parseWhileStmt() {
    int line = current().line;
    advance(); // skip 'while'
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after while condition");

    auto body = parseStatement();
    return makeWhile(std::move(cond), std::move(body), line);
}

StmtPtr Parser::parseDoWhileStmt() {
    int line = current().line;
    advance(); // skip 'do'
    auto body = parseStatement();
    expect(TokenType::KW_WHILE, "Expected 'while' after do body");
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    auto cond = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after do-while condition");
    expect(TokenType::SEMICOLON, "Expected ';' after do-while");
    return makeDoWhile(std::move(cond), std::move(body), line);
}

StmtPtr Parser::parseForStmt() {
    int line = current().line;
    advance(); // skip 'for'
    expect(TokenType::LPAREN, "Expected '(' after 'for'");

    // Determine for variant by scanning ahead
    // 1) for(type name : expr) — ForEach with type
    // 2) for(name : expr)      — ForEach without type (infer from iterable)
    // 3) for(expr)             — ForN (repeat n times) or chained comparison for
    // 4) for(init; cond; incr) — C-style

    // Check for ForEach: type ident ':'  or  ident ':'
    if (isTypeKeyword(current().type)) {
        // Could be ForEach with type OR C-style with type decl init
        // Look ahead: type ident ':' → ForEach, type ident '=' or ';' → C-style
        size_t saved = pos_;
        parseType(); // consume type (may be multi-token like int[])
        if (check(TokenType::IDENTIFIER)) {
            size_t afterName = pos_ + 1;
            if (afterName < tokens_.size() && tokens_[afterName].type == TokenType::COLON) {
                // ForEach: for(type name : expr)
                pos_ = saved;
                std::string type = parseType();
                std::string name = expect(TokenType::IDENTIFIER, "Expected identifier").value;
                expect(TokenType::COLON, "Expected ':' in for-each");
                auto iterable = parseExpression();
                expect(TokenType::RPAREN, "Expected ')' after for-each");
                auto body = parseStatement();
                return makeForEach(type, name, std::move(iterable), std::move(body), line);
            }
        }
        // Not ForEach — restore and fall through to C-style
        pos_ = saved;
    } else if (check(TokenType::IDENTIFIER)) {
        // Could be ForEach without type: for(x : arr)
        // Or could be C-style: for(x = 0; ...)
        // Or ForN: for(x)
        size_t saved = pos_;
        if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == TokenType::COLON) {
            // ForEach: for(name : expr)
            std::string name = advance().value;
            advance(); // skip ':'
            auto iterable = parseExpression();
            expect(TokenType::RPAREN, "Expected ')' after for-each");
            auto body = parseStatement();
            return makeForEach("", name, std::move(iterable), std::move(body), line);
        }
        pos_ = saved;
    }

    // Check if this is a simple for(expr) — no semicolons inside
    // Scan ahead to check for semicolons before closing paren
    {
        int parenDepth = 1;
        bool hasSemicolon = false;
        for (size_t i = pos_; i < tokens_.size() && parenDepth > 0; i++) {
            if (tokens_[i].type == TokenType::LPAREN) parenDepth++;
            else if (tokens_[i].type == TokenType::RPAREN) { parenDepth--; if (parenDepth == 0) break; }
            else if (tokens_[i].type == TokenType::SEMICOLON && parenDepth == 1) { hasSemicolon = true; break; }
        }

        if (!hasSemicolon) {
            // No semicolons — for(expr) variant
            auto expr = parseExpression();
            expect(TokenType::RPAREN, "Expected ')' after for expression");
            auto body = parseStatement();

            if (expr->kind == ExprKind::ChainedComparison) {
                // for(0 < i < 10) — desugar to C-style for
                // Require exactly 2 ops and middle operand must be identifier
                if (expr->chainOperands.size() == 3 && expr->chainOperands[1]->kind == ExprKind::Identifier) {
                    std::string varName = expr->chainOperands[1]->name;
                    TokenType op1 = expr->chainOps[0]; // e.g. <  in "0 < i"
                    TokenType op2 = expr->chainOps[1]; // e.g. <  in "i < 10"

                    // Determine direction and start value
                    ExprPtr startExpr, endExpr;
                    TokenType condOp;
                    bool ascending;

                    if (op1 == TokenType::LT || op1 == TokenType::LTE) {
                        // low < i < high  or  low <= i < high
                        ascending = true;
                        // start = low (+1 if strict <)
                        if (op1 == TokenType::LT) {
                            // 0 < i → i starts at 0+1
                            startExpr = makeBinaryOp(TokenType::PLUS, std::move(expr->chainOperands[0]),
                                                     makeIntLiteral(1, line), line);
                        } else {
                            // 0 <= i → i starts at 0
                            startExpr = std::move(expr->chainOperands[0]);
                        }
                        condOp = op2; // i < high or i <= high
                        endExpr = std::move(expr->chainOperands[2]);
                    } else {
                        // high > i > low  or  high >= i >= low
                        ascending = false;
                        if (op1 == TokenType::GT) {
                            // 10 > i → i starts at 10-1
                            startExpr = makeBinaryOp(TokenType::MINUS, std::move(expr->chainOperands[0]),
                                                     makeIntLiteral(1, line), line);
                        } else {
                            // 10 >= i → i starts at 10
                            startExpr = std::move(expr->chainOperands[0]);
                        }
                        condOp = op2; // i > low or i >= low
                        endExpr = std::move(expr->chainOperands[2]);
                    }

                    // Build: int varName = startExpr;
                    auto init = makeVarDecl("int", varName, std::move(startExpr), line);
                    // Build: varName condOp endExpr
                    auto cond = makeBinaryOp(condOp, makeIdentifier(varName, line), std::move(endExpr), line);
                    // Build: varName++ or varName--
                    ExprPtr incr;
                    if (ascending) {
                        auto inc = std::make_unique<Expr>();
                        inc->kind = ExprKind::PreIncrement;
                        inc->operand = makeIdentifier(varName, line);
                        inc->line = line;
                        incr = std::move(inc);
                    } else {
                        auto dec = std::make_unique<Expr>();
                        dec->kind = ExprKind::PreDecrement;
                        dec->operand = makeIdentifier(varName, line);
                        dec->line = line;
                        incr = std::move(dec);
                    }

                    return makeFor(std::move(init), std::move(cond), std::move(incr), std::move(body), line);
                } else {
                    error("Chained comparison for loop must have exactly one loop variable (e.g. for(0 < i < 10))");
                }
            }

            // Simple for(n) — repeat n times
            return makeForN(std::move(expr), std::move(body), line);
        }
    }

    // C-style for(init; cond; incr)
    StmtPtr init = nullptr;
    if (isTypeKeyword(current().type)) {
        std::string type = parseType();
        std::string name = expect(TokenType::IDENTIFIER, "Expected identifier").value;
        init = parseVarDecl(type);
    } else if (!check(TokenType::SEMICOLON)) {
        init = parseExprStatement();
    } else {
        advance(); // skip ;
    }

    ExprPtr cond = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        cond = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after for condition");

    ExprPtr incr = nullptr;
    if (!check(TokenType::RPAREN)) {
        incr = parseExpression();
    }
    expect(TokenType::RPAREN, "Expected ')' after for clauses");

    auto body = parseStatement();
    return makeFor(std::move(init), std::move(cond), std::move(incr), std::move(body), line);
}

StmtPtr Parser::parseSwitchStmt() {
    int line = current().line;
    advance(); // skip 'switch'
    expect(TokenType::LPAREN, "Expected '(' after 'switch'");
    auto switchExpr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after switch expression");
    expect(TokenType::LBRACE, "Expected '{' after switch(...)");

    std::vector<ExprPtr> caseValues;
    std::vector<std::vector<StmtPtr>> caseBodies;

    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        if (match(TokenType::KW_CASE)) {
            auto val = parseExpression();
            expect(TokenType::COLON, "Expected ':' after case value");
            caseValues.push_back(std::move(val));
        } else if (match(TokenType::KW_DEFAULT)) {
            expect(TokenType::COLON, "Expected ':' after 'default'");
            caseValues.push_back(nullptr); // nullptr = default
        } else {
            error("Expected 'case' or 'default' in switch body");
        }

        std::vector<StmtPtr> stmts;
        while (!check(TokenType::KW_CASE) && !check(TokenType::KW_DEFAULT) &&
               !check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
            stmts.push_back(parseStatement());
        }
        caseBodies.push_back(std::move(stmts));
    }

    expect(TokenType::RBRACE, "Expected '}' to close switch");
    return makeSwitch(std::move(switchExpr), std::move(caseValues), std::move(caseBodies), line);
}

StmtPtr Parser::parseReturnStmt() {
    int line = current().line;
    advance(); // skip 'return'

    ExprPtr expr = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        expr = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after return");
    return makeReturn(std::move(expr), line);
}

StmtPtr Parser::parsePrintStmt() {
    int line = current().line;
    bool newline = (current().type == TokenType::KW_PRINTLN);
    advance(); // skip 'print' or 'println'
    expect(TokenType::LPAREN, "Expected '(' after 'print'/'println'");

    std::vector<ExprPtr> args;
    if (!check(TokenType::RPAREN)) {
        args.push_back(parseExpression());
        while (match(TokenType::COMMA)) {
            args.push_back(parseExpression());
        }
    }
    expect(TokenType::RPAREN, "Expected ')' after print arguments");
    expect(TokenType::SEMICOLON, "Expected ';' after print statement");

    return makePrint(std::move(args), line, newline);
}

StmtPtr Parser::parseExprStatement() {
    int line = current().line;
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return makeExprStmt(std::move(expr), line);
}

// ============== Expressions ==============

ExprPtr Parser::parseExpression() {
    return parseAssignment();
}

ExprPtr Parser::parseAssignment() {
    auto expr = parseOr();

    if (expr->kind == ExprKind::Identifier) {
        if (check(TokenType::ASSIGN)) {
            advance();
            auto value = parseAssignment();
            return makeAssign(expr->name, std::move(value), expr->line);
        }
        if (check(TokenType::PLUS_ASSIGN) || check(TokenType::MINUS_ASSIGN) ||
            check(TokenType::STAR_ASSIGN) || check(TokenType::SLASH_ASSIGN) ||
            check(TokenType::PERCENT_ASSIGN)) {
            TokenType op = advance().type;
            auto value = parseAssignment();
            return makeCompoundAssign(expr->name, op, std::move(value), expr->line);
        }
    }

    // Array element assignment: arr[i] = value, arr[i] += value
    if (expr->kind == ExprKind::ArrayAccess) {
        if (check(TokenType::ASSIGN)) {
            advance();
            auto value = parseAssignment();
            return makeArrayAssign(std::move(expr->arrayExpr), std::move(expr->indexExpr),
                                   std::move(value), expr->line);
        }
        if (check(TokenType::PLUS_ASSIGN) || check(TokenType::MINUS_ASSIGN) ||
            check(TokenType::STAR_ASSIGN) || check(TokenType::SLASH_ASSIGN) ||
            check(TokenType::PERCENT_ASSIGN)) {
            TokenType op = advance().type;
            auto value = parseAssignment();
            return makeArrayCompoundAssign(std::move(expr->arrayExpr), std::move(expr->indexExpr),
                                           op, std::move(value), expr->line);
        }
    }

    return expr;
}

ExprPtr Parser::parseOr() {
    auto left = parseAnd();
    while (check(TokenType::OR)) {
        int line = current().line;
        advance();
        auto right = parseAnd();
        left = makeBinaryOp(TokenType::OR, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    auto left = parseEquality();
    while (check(TokenType::AND)) {
        int line = current().line;
        advance();
        auto right = parseEquality();
        left = makeBinaryOp(TokenType::AND, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        TokenType op = advance().type;
        int line = current().line;
        auto right = parseComparison();
        left = makeBinaryOp(op, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parseComparison() {
    auto first = parseAddition();
    if (!check(TokenType::LT) && !check(TokenType::GT) &&
        !check(TokenType::LTE) && !check(TokenType::GTE)) {
        return first;
    }

    // At least one comparison operator
    TokenType op1 = advance().type;
    int line = current().line;
    auto second = parseAddition();

    if (!check(TokenType::LT) && !check(TokenType::GT) &&
        !check(TokenType::LTE) && !check(TokenType::GTE)) {
        // Single comparison, return normal BinaryOp
        return makeBinaryOp(op1, std::move(first), std::move(second), line);
    }

    // Chained comparison: collect all operands and ops
    std::vector<ExprPtr> operands;
    std::vector<TokenType> ops;
    operands.push_back(std::move(first));
    operands.push_back(std::move(second));
    ops.push_back(op1);

    while (check(TokenType::LT) || check(TokenType::GT) ||
           check(TokenType::LTE) || check(TokenType::GTE)) {
        ops.push_back(advance().type);
        operands.push_back(parseAddition());
    }

    return makeChainedComparison(std::move(operands), std::move(ops), line);
}

ExprPtr Parser::parseAddition() {
    auto left = parseMultiplication();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op = advance().type;
        int line = current().line;
        auto right = parseMultiplication();
        left = makeBinaryOp(op, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parseMultiplication() {
    auto left = parsePower();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT) || check(TokenType::DIVMOD)) {
        TokenType op = advance().type;
        int line = current().line;
        auto right = parseUnary();
        left = makeBinaryOp(op, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parsePower() {
    auto left = parseUnary();
    if (check(TokenType::POWER)) {
        int line = current().line;
        advance();
        auto right = parsePower(); // right-associative: 2**3**2 = 2**(3**2)
        left = makeBinaryOp(TokenType::POWER, std::move(left), std::move(right), line);
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::MINUS) || check(TokenType::NOT)) {
        TokenType op = advance().type;
        int line = current().line;
        auto operand = parseUnary();
        return makeUnaryOp(op, std::move(operand), line);
    }

    // Pre-increment / pre-decrement
    if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
        TokenType op = advance().type;
        int line = current().line;
        auto operand = parsePostfix();
        auto e = std::make_unique<Expr>();
        e->kind = (op == TokenType::INCREMENT) ? ExprKind::PreIncrement : ExprKind::PreDecrement;
        e->operand = std::move(operand);
        e->line = line;
        return e;
    }

    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (check(TokenType::LBRACKET)) {
            int line = expr->line;
            advance();
            // Check for slice: [start:end]
            auto startExpr = parseExpression();
            if (check(TokenType::COLON)) {
                advance();
                auto endExpr = parseExpression();
                expect(TokenType::RBRACKET, "Expected ']' after slice");
                expr = makeArraySlice(std::move(expr), std::move(startExpr), std::move(endExpr), line);
            } else {
                expect(TokenType::RBRACKET, "Expected ']' after array index");
                expr = makeArrayAccess(std::move(expr), std::move(startExpr), line);
            }
        } else if (check(TokenType::DOT)) {
            // Method call: expr.func(args) → func(expr, args)
            advance(); // consume '.'
            Token methodName = expect(TokenType::IDENTIFIER, "Expected method name after '.'");
            expect(TokenType::LPAREN, "Expected '(' after method name");
            std::vector<ExprPtr> args;
            args.push_back(std::move(expr)); // receiver becomes first argument
            if (!check(TokenType::RPAREN)) {
                args.push_back(parseExpression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after method arguments");
            expr = makeFunctionCall(methodName.value, std::move(args), methodName.line);
        } else if (check(TokenType::INCREMENT)) {
            advance();
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::PostIncrement;
            e->operand = std::move(expr);
            e->line = current().line;
            expr = std::move(e);
        } else if (check(TokenType::DECREMENT)) {
            advance();
            auto e = std::make_unique<Expr>();
            e->kind = ExprKind::PostDecrement;
            e->operand = std::move(expr);
            e->line = current().line;
            expr = std::move(e);
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::parsePrimary() {
    // Number literals
    if (check(TokenType::INT_LIT)) {
        Token tok = advance();
        return makeIntLiteral(std::stoll(tok.value), tok.line);
    }
    if (check(TokenType::FLOAT_LIT)) {
        Token tok = advance();
        return makeFloatLiteral(std::stod(tok.value), tok.line);
    }

    // String literal
    if (check(TokenType::STRING_LIT)) {
        Token tok = advance();
        return makeStringLiteral(tok.value, tok.line);
    }

    // F-string literal
    if (check(TokenType::FSTRING_LIT)) {
        return parseFStringExpr();
    }

    // Char literal
    if (check(TokenType::CHAR_LIT)) {
        Token tok = advance();
        return makeCharLiteral(tok.value[0], tok.line);
    }

    // Bool literal
    if (check(TokenType::BOOL_LIT)) {
        Token tok = advance();
        return makeBoolLiteral(tok.value == "true", tok.line);
    }

    // input()
    if (check(TokenType::KW_INPUT)) {
        int line = current().line;
        advance();
        expect(TokenType::LPAREN, "Expected '(' after 'input'");
        expect(TokenType::RPAREN, "Expected ')' after 'input('");
        return makeInput(line);
    }

    // Type cast: int(expr), float32(expr), etc.
    if (isTypeKeyword(current().type) && peek().type == TokenType::LPAREN) {
        Token tok = advance(); // consume type keyword
        advance(); // consume '('
        std::vector<ExprPtr> args;
        if (!check(TokenType::RPAREN)) {
            args.push_back(parseExpression());
            while (match(TokenType::COMMA)) {
                args.push_back(parseExpression());
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after type cast arguments");
        return makeFunctionCall(tok.value, std::move(args), tok.line);
    }

    // Type method: int.max(), float32.min(), etc.
    if (isTypeKeyword(current().type) && peek().type == TokenType::DOT) {
        Token typeTok = advance(); // consume type keyword
        advance(); // consume '.'
        Token methodTok = expect(TokenType::IDENTIFIER, "Expected method name after '.'");
        expect(TokenType::LPAREN, "Expected '(' after type method name");
        expect(TokenType::RPAREN, "Expected ')' after type method call");
        std::vector<ExprPtr> args;
        args.push_back(makeStringLiteral(typeTok.value, typeTok.line));
        args.push_back(makeStringLiteral(methodTok.value, methodTok.line));
        return makeFunctionCall("__type_method", std::move(args), typeTok.line);
    }

    // Identifier or function call
    if (check(TokenType::IDENTIFIER)) {
        Token tok = advance();

        // Function call
        if (check(TokenType::LPAREN)) {
            advance();
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                args.push_back(parseExpression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            return makeFunctionCall(tok.value, std::move(args), tok.line);
        }

        return makeIdentifier(tok.value, tok.line);
    }

    // Parenthesized expression
    if (check(TokenType::LPAREN)) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    // Array literal
    if (check(TokenType::LBRACKET)) {
        int line = current().line;
        advance();
        std::vector<ExprPtr> elements;
        if (!check(TokenType::RBRACKET)) {
            elements.push_back(parseExpression());
            while (match(TokenType::COMMA)) {
                elements.push_back(parseExpression());
            }
        }
        expect(TokenType::RBRACKET, "Expected ']' after array literal");
        return makeArrayLiteral(std::move(elements), line);
    }

    error("Unexpected token '" + current().value + "' at line " + std::to_string(current().line));
}

ExprPtr Parser::parseFStringExpr() {
    Token tok = advance();
    const std::string& raw = tok.value;
    std::vector<ExprPtr> parts;

    auto processEscapes = [](const std::string& s) -> std::string {
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                switch (s[i + 1]) {
                    case 'n': result += '\n'; i++; break;
                    case 't': result += '\t'; i++; break;
                    case '\\': result += '\\'; i++; break;
                    case '"': result += '"'; i++; break;
                    case '0': result += '\0'; i++; break;
                    case 'r': result += '\r'; i++; break;
                    default: result += s[i + 1]; i++; break;
                }
            } else {
                result += s[i];
            }
        }
        return result;
    };

    std::string textPart;
    size_t i = 0;
    while (i < raw.size()) {
        if (raw[i] == '{') {
            // Flush accumulated text
            if (!textPart.empty()) {
                parts.push_back(makeStringLiteral(processEscapes(textPart), tok.line));
                textPart.clear();
            }
            i++; // skip {
            // Extract expression string
            std::string exprStr;
            int depth = 1;
            while (i < raw.size() && depth > 0) {
                if (raw[i] == '{') depth++;
                else if (raw[i] == '}') { depth--; if (depth == 0) break; }
                exprStr += raw[i];
                i++;
            }
            if (i < raw.size()) i++; // skip closing }

            // Sub-lex and sub-parse the expression
            Lexer subLexer(exprStr);
            auto subTokens = subLexer.tokenize();
            Parser subParser(subTokens);
            parts.push_back(subParser.parseExpression());
        } else if (raw[i] == '\\') {
            textPart += raw[i];
            i++;
            if (i < raw.size()) {
                textPart += raw[i];
                i++;
            }
        } else {
            textPart += raw[i];
            i++;
        }
    }
    // Flush remaining text
    if (!textPart.empty()) {
        parts.push_back(makeStringLiteral(processEscapes(textPart), tok.line));
    }

    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::FStringExpr;
    e->elements = std::move(parts);
    e->line = tok.line;
    return e;
}
