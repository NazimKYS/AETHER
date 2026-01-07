// src/PathCondition.h
#ifndef PATH_CONDITION_H
#define PATH_CONDITION_H

#include "clang/AST/Expr.h"        // ← added: needed for Expr::dump()
#include "clang/AST/Stmt.h"
#include "clang/AST/Decl.h"
#include <vector>

// No need to re-forward-declare if included
struct PathConditionElement {
    const clang::Expr* condition = nullptr;
    bool isTrueBranch = true;
};

struct RawPathCondition {
    bool found = false;
    const clang::Stmt* target = nullptr;
    std::vector<PathConditionElement> conditionStack;
    void dump() const;
};

RawPathCondition collectPathToTarget(const clang::FunctionDecl* func, const clang::Stmt* target);

#endif