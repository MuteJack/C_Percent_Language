// ast.h
/*
Abstract Syntax Tree (AST) node definitions.
Two kinds of nodes exist: Expr (expressions) and Stmt (statements),
distinguished by ExprKind / StmtKind enums.
Only specific fields of the Expr/Stmt structs are used depending on the node kind.
AST nodes are created via make* factory functions.
*/

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "token.h"

// Forward declarations
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ============== Expressions ==============

enum class ExprKind {
    IntLiteral, BigIntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral,
    Identifier, BinaryOp, UnaryOp, Assign, CompoundAssign,
    FunctionCall, ArrayAccess, ArrayLiteral,
    PreIncrement, PreDecrement, PostIncrement, PostDecrement,
    FStringExpr,
    Input,
    ArrayAssign, ArrayCompoundAssign, ArraySlice,
    DictLiteral,
    ChainedComparison,
    Ternary,
};

// Expression node — all ExprKinds share a single struct (tagged union style).
// Active fields depend on kind:
//   IntLiteral/CharLiteral: intVal
//   FloatLiteral: floatVal
//   StringLiteral/BigIntLiteral: strVal
//   BoolLiteral: boolVal
//   Identifier/Assign/CompoundAssign: name
//   BinaryOp: op, left, right
//   UnaryOp/PreIncrement/PreDecrement: op, operand
//   FunctionCall: funcName, args
//   ArrayAccess: arrayExpr, indexExpr
//   ArraySlice: arrayExpr, indexExpr, endIndexExpr
//   ArrayLiteral/FStringExpr: elements
//   DictLiteral: elements(keys), dictValues(values)
//   ChainedComparison: chainOperands, chainOps
//   Ternary: operand(condition), left(true branch), right(false branch)
struct Expr {
    ExprKind kind;
    int line;

    // Literal values
    int64_t intVal;
    double floatVal;
    std::string strVal;
    bool boolVal;

    // Identifier name (used by Identifier, Assign, CompoundAssign)
    std::string name;

    // Binary/unary operations
    TokenType op;
    ExprPtr left;
    ExprPtr right;
    ExprPtr operand; // unary operand

    // Function call
    std::string funcName;
    std::vector<ExprPtr> args;

    // Array/dictionary related
    ExprPtr arrayExpr;    // target expression for array access
    ExprPtr indexExpr;     // index expression
    ExprPtr endIndexExpr;  // slice end index [start:end]
    std::vector<ExprPtr> elements;    // array literal elements / dict keys
    std::vector<ExprPtr> dictValues;  // dict values

    // Chained comparison: a < b < c → operands [a,b,c], ops [<,<]
    std::vector<ExprPtr> chainOperands;
    std::vector<TokenType> chainOps;

    Expr() : kind(ExprKind::IntLiteral), line(0), intVal(0), floatVal(0.0),
             boolVal(false), op(TokenType::ERROR) {}
};

// AST node factory functions — conveniently create nodes for each ExprKind/StmtKind
inline ExprPtr makeIntLiteral(int64_t val, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::IntLiteral;
    e->intVal = val;
    e->line = line;
    return e;
}

inline ExprPtr makeFloatLiteral(double val, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::FloatLiteral;
    e->floatVal = val;
    e->line = line;
    return e;
}

inline ExprPtr makeStringLiteral(const std::string& val, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::StringLiteral;
    e->strVal = val;
    e->line = line;
    return e;
}

inline ExprPtr makeCharLiteral(char val, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::CharLiteral;
    e->intVal = static_cast<int64_t>(val);
    e->line = line;
    return e;
}

inline ExprPtr makeBoolLiteral(bool val, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::BoolLiteral;
    e->boolVal = val;
    e->line = line;
    return e;
}

inline ExprPtr makeIdentifier(const std::string& name, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::Identifier;
    e->name = name;
    e->line = line;
    return e;
}

inline ExprPtr makeBinaryOp(TokenType op, ExprPtr left, ExprPtr right, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::BinaryOp;
    e->op = op;
    e->left = std::move(left);
    e->right = std::move(right);
    e->line = line;
    return e;
}

inline ExprPtr makeUnaryOp(TokenType op, ExprPtr operand, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::UnaryOp;
    e->op = op;
    e->operand = std::move(operand);
    e->line = line;
    return e;
}

inline ExprPtr makeAssign(const std::string& name, ExprPtr value, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::Assign;
    e->name = name;
    e->right = std::move(value);
    e->line = line;
    return e;
}

inline ExprPtr makeCompoundAssign(const std::string& name, TokenType op, ExprPtr value, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::CompoundAssign;
    e->name = name;
    e->op = op;
    e->right = std::move(value);
    e->line = line;
    return e;
}

inline ExprPtr makeFunctionCall(const std::string& name, std::vector<ExprPtr> args, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::FunctionCall;
    e->funcName = name;
    e->args = std::move(args);
    e->line = line;
    return e;
}

inline ExprPtr makeArrayAccess(ExprPtr array, ExprPtr index, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ArrayAccess;
    e->arrayExpr = std::move(array);
    e->indexExpr = std::move(index);
    e->line = line;
    return e;
}

inline ExprPtr makeArrayLiteral(std::vector<ExprPtr> elements, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ArrayLiteral;
    e->elements = std::move(elements);
    e->line = line;
    return e;
}

inline ExprPtr makeDictLiteral(std::vector<ExprPtr> keys, std::vector<ExprPtr> values, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::DictLiteral;
    e->elements = std::move(keys);
    e->dictValues = std::move(values);
    e->line = line;
    return e;
}

inline ExprPtr makeArrayAssign(ExprPtr array, ExprPtr index, ExprPtr value, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ArrayAssign;
    e->arrayExpr = std::move(array);
    e->indexExpr = std::move(index);
    e->right = std::move(value);
    e->line = line;
    return e;
}

inline ExprPtr makeArrayCompoundAssign(ExprPtr array, ExprPtr index, TokenType op, ExprPtr value, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ArrayCompoundAssign;
    e->arrayExpr = std::move(array);
    e->indexExpr = std::move(index);
    e->op = op;
    e->right = std::move(value);
    e->line = line;
    return e;
}

inline ExprPtr makeArraySlice(ExprPtr array, ExprPtr start, ExprPtr end, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ArraySlice;
    e->arrayExpr = std::move(array);
    e->indexExpr = std::move(start);
    e->endIndexExpr = std::move(end);
    e->line = line;
    return e;
}

inline ExprPtr makeInput(int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::Input;
    e->line = line;
    return e;
}

inline ExprPtr makeChainedComparison(std::vector<ExprPtr> operands, std::vector<TokenType> ops, int line) {
    auto e = std::make_unique<Expr>();
    e->kind = ExprKind::ChainedComparison;
    e->chainOperands = std::move(operands);
    e->chainOps = std::move(ops);
    e->line = line;
    return e;
}

// ============== Statements ==============

enum class StmtKind {
    VarDecl, ExprStmt, Block, If, While, DoWhile, For,
    FuncDecl, Return, Print, Break, Continue,
    ForEach, ForN, Switch,
};

struct FuncParam {
    std::string type;
    std::string name;
};

// Statement node — like Expr, active fields depend on StmtKind.
//   VarDecl: varType, varName, initExpr, arrayDimSizes
//   ExprStmt: expr
//   Block / FuncDecl: body
//   If / While / DoWhile: condition, thenBranch, elseBranch
//   For: initStmt, condition, increment, thenBranch
//   ForEach: varType, varName, expr(iterable), thenBranch
//   ForN: expr(repeat count), thenBranch
//   FuncDecl: funcReturnType, funcName, params, body
//   Print: printArgs, printNewline
//   Switch: condition, caseValues, caseBodies
struct Stmt {
    StmtKind kind;
    int line;

    // Variable declaration
    std::string varType;
    std::string varName;
    ExprPtr initExpr;
    std::vector<ExprPtr> arrayDimSizes; // dimension sizes for array declaration (nullptr = inferred)

    // Expression statement
    ExprPtr expr;

    // Block / function body
    std::vector<StmtPtr> body;

    // Conditionals / loops
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    // for loop
    StmtPtr initStmt;   // initializer
    ExprPtr increment;   // increment expression

    // Function declaration
    std::string funcReturnType;
    std::string funcName;
    std::vector<FuncParam> params;

    // print / println
    std::vector<ExprPtr> printArgs;
    bool printNewline = false; // true for println

    // switch-case
    std::vector<ExprPtr> caseValues;         // nullptr = default
    std::vector<std::vector<StmtPtr>> caseBodies;

    Stmt() : kind(StmtKind::ExprStmt), line(0) {}
};

// Statement node factory functions
inline StmtPtr makeVarDecl(const std::string& type, const std::string& name, ExprPtr init, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::VarDecl;
    s->varType = type;
    s->varName = name;
    s->initExpr = std::move(init);
    s->line = line;
    return s;
}

inline StmtPtr makeExprStmt(ExprPtr expr, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ExprStmt;
    s->expr = std::move(expr);
    s->line = line;
    return s;
}

inline StmtPtr makeBlock(std::vector<StmtPtr> stmts, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Block;
    s->body = std::move(stmts);
    s->line = line;
    return s;
}

inline StmtPtr makeIf(ExprPtr cond, StmtPtr thenB, StmtPtr elseB, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::If;
    s->condition = std::move(cond);
    s->thenBranch = std::move(thenB);
    s->elseBranch = std::move(elseB);
    s->line = line;
    return s;
}

inline StmtPtr makeWhile(ExprPtr cond, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::While;
    s->condition = std::move(cond);
    s->thenBranch = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeDoWhile(ExprPtr cond, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::DoWhile;
    s->condition = std::move(cond);
    s->thenBranch = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeFor(StmtPtr init, ExprPtr cond, ExprPtr incr, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::For;
    s->initStmt = std::move(init);
    s->condition = std::move(cond);
    s->increment = std::move(incr);
    s->thenBranch = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeFuncDecl(const std::string& retType, const std::string& name,
                            std::vector<FuncParam> params, std::vector<StmtPtr> body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::FuncDecl;
    s->funcReturnType = retType;
    s->funcName = name;
    s->params = std::move(params);
    s->body = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeReturn(ExprPtr expr, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Return;
    s->expr = std::move(expr);
    s->line = line;
    return s;
}

inline StmtPtr makePrint(std::vector<ExprPtr> args, int line, bool newline = false) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Print;
    s->printArgs = std::move(args);
    s->printNewline = newline;
    s->line = line;
    return s;
}

inline StmtPtr makeBreak(int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Break;
    s->line = line;
    return s;
}

inline StmtPtr makeContinue(int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Continue;
    s->line = line;
    return s;
}

inline StmtPtr makeForEach(const std::string& type, const std::string& name,
                           ExprPtr iterable, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ForEach;
    s->varType = type;
    s->varName = name;
    s->expr = std::move(iterable);
    s->thenBranch = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeForN(ExprPtr count, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::ForN;
    s->expr = std::move(count);
    s->thenBranch = std::move(body);
    s->line = line;
    return s;
}

inline StmtPtr makeSwitch(ExprPtr switchExpr,
                           std::vector<ExprPtr> caseVals,
                           std::vector<std::vector<StmtPtr>> caseBods, int line) {
    auto s = std::make_unique<Stmt>();
    s->kind = StmtKind::Switch;
    s->condition = std::move(switchExpr);
    s->caseValues = std::move(caseVals);
    s->caseBodies = std::move(caseBods);
    s->line = line;
    return s;
}
