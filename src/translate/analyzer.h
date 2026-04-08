// analyzer.h
// Static analysis pass — runs on AST before translation to C++.
// Currently implements:
//   - Move-after-use detection: flags use of variables after `let` ownership transfer.
#pragma once
#include "../core/ast.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class AnalysisError : public std::runtime_error {
public:
    AnalysisError(const std::string& msg, int line)
        : std::runtime_error("Analysis error at line " + std::to_string(line) + ": " + msg),
          line_(line) {}
    int line() const { return line_; }
private:
    int line_;
};

class Analyzer {
public:
    // Runs all analysis passes on the program. Throws AnalysisError on failure.
    void analyze(const std::vector<StmtPtr>& program) {
        errors_.clear();
        // Global scope
        Scope global;
        for (auto& stmt : program) {
            analyzeStmt(stmt.get(), global);
        }
        if (!errors_.empty()) {
            // Throw the first error (could collect all and report)
            throw AnalysisError(errors_[0].message, errors_[0].line);
        }
    }

    // Returns all collected errors without throwing.
    struct Error {
        std::string message;
        int line;
    };

    const std::vector<Error>& getErrors() const { return errors_; }

    // Analyze and collect errors without throwing
    bool check(const std::vector<StmtPtr>& program) {
        errors_.clear();
        Scope global;
        for (auto& stmt : program) {
            analyzeStmt(stmt.get(), global);
        }
        return errors_.empty();
    }

private:
    std::vector<Error> errors_;

    // Tracks moved variables per scope
    struct Scope {
        std::unordered_set<std::string> moved;  // variables that have been moved
        Scope* parent = nullptr;

        bool isMoved(const std::string& name) const {
            if (moved.count(name)) return true;
            if (parent) return parent->isMoved(name);
            return false;
        }

        void markMoved(const std::string& name) {
            moved.insert(name);
        }

        // When a variable is reassigned, it's no longer moved
        void unmarkMoved(const std::string& name) {
            moved.erase(name);
        }
    };

    // ============== Statement Analysis ==============

    void analyzeStmt(const Stmt* stmt, Scope& scope) {
        if (!stmt) return;

        switch (stmt->kind) {
            case StmtKind::VarDecl:
                analyzeVarDecl(stmt, scope);
                break;
            case StmtKind::ExprStmt:
                analyzeExpr(stmt->expr.get(), scope);
                break;
            case StmtKind::Block: {
                Scope child;
                child.parent = &scope;
                for (auto& s : stmt->body) analyzeStmt(s.get(), child);
                break;
            }
            case StmtKind::If:
                analyzeExpr(stmt->condition.get(), scope);
                if (stmt->thenBranch) analyzeStmt(stmt->thenBranch.get(), scope);
                if (stmt->elseBranch) analyzeStmt(stmt->elseBranch.get(), scope);
                break;
            case StmtKind::While:
            case StmtKind::DoWhile:
                analyzeExpr(stmt->condition.get(), scope);
                if (stmt->thenBranch) analyzeStmt(stmt->thenBranch.get(), scope);
                break;
            case StmtKind::For:
                if (stmt->initStmt) analyzeStmt(stmt->initStmt.get(), scope);
                analyzeExpr(stmt->condition.get(), scope);
                analyzeExpr(stmt->increment.get(), scope);
                if (stmt->thenBranch) analyzeStmt(stmt->thenBranch.get(), scope);
                break;
            case StmtKind::ForEach:
                analyzeExpr(stmt->expr.get(), scope);
                if (stmt->thenBranch) analyzeStmt(stmt->thenBranch.get(), scope);
                break;
            case StmtKind::ForN:
                analyzeExpr(stmt->expr.get(), scope);
                if (stmt->thenBranch) analyzeStmt(stmt->thenBranch.get(), scope);
                break;
            case StmtKind::FuncDecl: {
                // Function body gets its own scope
                Scope funcScope;
                funcScope.parent = &scope;
                for (auto& s : stmt->body) analyzeStmt(s.get(), funcScope);
                break;
            }
            case StmtKind::Return:
                analyzeExpr(stmt->expr.get(), scope);
                break;
            case StmtKind::Print:
                for (auto& arg : stmt->printArgs) analyzeExpr(arg.get(), scope);
                break;
            case StmtKind::Switch:
                analyzeExpr(stmt->condition.get(), scope);
                for (auto& body : stmt->caseBodies) {
                    for (auto& s : body) analyzeStmt(s.get(), scope);
                }
                break;
            case StmtKind::Break:
            case StmtKind::Continue:
                break;
        }
    }

    void analyzeVarDecl(const Stmt* stmt, Scope& scope) {
        // let qualifier: `let int y = x;` — mark x as moved
        if (stmt->isLet && stmt->initExpr) {
            if (stmt->initExpr->kind == ExprKind::Identifier) {
                const std::string& source = stmt->initExpr->name;
                // Check if source is already moved
                if (scope.isMoved(source)) {
                    addError("Variable '" + source + "' has already been moved", stmt->line);
                }
                scope.markMoved(source);
            } else {
                // Analyze the init expression normally
                analyzeExpr(stmt->initExpr.get(), scope);
            }
        } else if (stmt->initExpr) {
            analyzeExpr(stmt->initExpr.get(), scope);
        }
    }

    // ============== Expression Analysis ==============

    void analyzeExpr(const Expr* expr, Scope& scope) {
        if (!expr) return;

        switch (expr->kind) {
            case ExprKind::Identifier:
                // Check if this variable has been moved
                if (scope.isMoved(expr->name)) {
                    addError("Variable '" + expr->name + "' has been moved and cannot be used", expr->line);
                }
                break;

            case ExprKind::Assign:
                // Reassignment to a moved variable revives it
                analyzeExpr(expr->right.get(), scope);
                scope.unmarkMoved(expr->name);
                break;

            case ExprKind::CompoundAssign:
                // Using a moved variable in compound assign is an error
                if (scope.isMoved(expr->name)) {
                    addError("Variable '" + expr->name + "' has been moved and cannot be used", expr->line);
                }
                analyzeExpr(expr->right.get(), scope);
                break;

            case ExprKind::BinaryOp:
                analyzeExpr(expr->left.get(), scope);
                analyzeExpr(expr->right.get(), scope);
                break;

            case ExprKind::UnaryOp:
                analyzeExpr(expr->operand.get(), scope);
                break;

            case ExprKind::PreIncrement:
            case ExprKind::PreDecrement:
                analyzeExpr(expr->operand.get(), scope);
                break;

            case ExprKind::PostIncrement:
            case ExprKind::PostDecrement:
                analyzeExpr(expr->operand.get(), scope);
                break;

            case ExprKind::FunctionCall:
                for (auto& arg : expr->args) {
                    if (arg->kind == ExprKind::LetArg) {
                        // let argument: mark source as moved
                        if (scope.isMoved(arg->name)) {
                            addError("Variable '" + arg->name + "' has already been moved", arg->line);
                        }
                        scope.markMoved(arg->name);
                    } else {
                        analyzeExpr(arg.get(), scope);
                    }
                }
                break;

            case ExprKind::ArrayAccess:
                analyzeExpr(expr->arrayExpr.get(), scope);
                analyzeExpr(expr->indexExpr.get(), scope);
                break;

            case ExprKind::ArrayAssign:
                analyzeExpr(expr->arrayExpr.get(), scope);
                analyzeExpr(expr->indexExpr.get(), scope);
                analyzeExpr(expr->right.get(), scope);
                break;

            case ExprKind::ArrayCompoundAssign:
                analyzeExpr(expr->arrayExpr.get(), scope);
                analyzeExpr(expr->indexExpr.get(), scope);
                analyzeExpr(expr->right.get(), scope);
                break;

            case ExprKind::ArraySlice:
                analyzeExpr(expr->arrayExpr.get(), scope);
                analyzeExpr(expr->indexExpr.get(), scope);
                analyzeExpr(expr->endIndexExpr.get(), scope);
                break;

            case ExprKind::ArrayLiteral:
                for (auto& el : expr->elements) analyzeExpr(el.get(), scope);
                break;

            case ExprKind::DictLiteral:
                for (auto& el : expr->elements) analyzeExpr(el.get(), scope);
                for (auto& v : expr->dictValues) analyzeExpr(v.get(), scope);
                break;

            case ExprKind::FStringExpr:
                for (auto& el : expr->elements) analyzeExpr(el.get(), scope);
                break;

            case ExprKind::ChainedComparison:
                for (auto& op : expr->chainOperands) analyzeExpr(op.get(), scope);
                break;

            case ExprKind::Ternary:
                analyzeExpr(expr->operand.get(), scope);
                analyzeExpr(expr->left.get(), scope);
                analyzeExpr(expr->right.get(), scope);
                break;

            case ExprKind::RefArg:
                if (scope.isMoved(expr->name)) {
                    addError("Variable '" + expr->name + "' has been moved and cannot be used", expr->line);
                }
                break;

            case ExprKind::LetArg:
                // Handled in FunctionCall
                break;

            // Literals — no variables to check
            case ExprKind::IntLiteral:
            case ExprKind::BigIntLiteral:
            case ExprKind::FloatLiteral:
            case ExprKind::StringLiteral:
            case ExprKind::CharLiteral:
            case ExprKind::BoolLiteral:
            case ExprKind::Input:
                break;
        }
    }

    void addError(const std::string& msg, int line) {
        errors_.push_back({msg, line});
    }
};
