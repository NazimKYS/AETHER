// PathCondition.cpp

#include "main.h"





// Helper: recursive, internal-only → keep static
static std::optional<RawPathCondition>
collectPathHelper(const Stmt* stmt, const Stmt* target,
                  std::vector<PathConditionElement> currentPath);

// PUBLIC function: NO 'static'!
RawPathCondition collectPathToTarget(const FunctionDecl* func, const Stmt* target) {
    if (!func || !func->getBody() || !target)
        return {};

    auto result = collectPathHelper(func->getBody(), target, {});
    if (result) {
        result->found = true;
        result->target = target;
        return std::move(*result);
    }
    return {};
}

// PRIVATE helper (static is OK here)
static std::optional<RawPathCondition>
collectPathHelper(const Stmt* stmt, const Stmt* target,
                  std::vector<PathConditionElement> currentPath) {
    if (!stmt) return std::nullopt;

    if (stmt == target) {
        RawPathCondition result;
        result.conditionStack = std::move(currentPath);
        return result;
    }

    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt)) {
        const Expr* cond = ifStmt->getCond();
        if (!cond) return std::nullopt;

        // Then branch
        if (const Stmt* thenBody = ifStmt->getThen()) {
            auto thenPath = currentPath;
            thenPath.push_back({cond, true});
            if (auto result = collectPathHelper(thenBody, target, thenPath))
                return result;
        }

        // Else branch
        if (const Stmt* elseBody = ifStmt->getElse()) {
            auto elsePath = currentPath;
            elsePath.push_back({cond, false});
            if (auto result = collectPathHelper(elseBody, target, elsePath))
                return result;
        }

        // Also check inside condition itself
        if (auto result = collectPathHelper(cond, target, currentPath))
            return result;

        return std::nullopt;
    }

    // Recurse into children
    for (const auto* child : stmt->children()) {
        if (auto result = collectPathHelper(child, target, currentPath))
            return result;
    }

    return std::nullopt;
}

// ✅ ADD THIS: definition of dump()
void RawPathCondition::dump() const {
    llvm::outs() << "Path to target:\n";
    for (const auto& elem : conditionStack) {
        llvm::outs() << "  ";
        if (!elem.isTrueBranch) {
            llvm::outs() << "!";
        }
        if (elem.condition) {
            elem.condition->dump(); // or dumpColor() if available
        } else {
            llvm::outs() << "(null condition)\n";
        }
    }
}