#pragma once
#include "main.h"

class ConditionPathAstVisitor : public RecursiveASTVisitor<ConditionPathAstVisitor> {
public:
    TargetProgramPoint targetPoint;
    ASTContext &astContext;
    SourceManager &srcmgr;

    explicit ConditionPathAstVisitor(SourceManager& _srcmgr, Rewriter& /*rewriter*/, ASTContext& Ctx)
        : astContext(Ctx), srcmgr(_srcmgr)
    {}

    bool VisitStmt(Stmt *currentStmt) {
        FullSourceLoc loc = astContext.getFullLoc(currentStmt->getBeginLoc());

        if (astContext.getSourceManager().isInMainFile(loc) &&
            loc.getSpellingLineNumber() == sourceLineOfTargetStmt) {

            if (isa<BinaryOperator>(currentStmt) ||
                isa<CallExpr>(currentStmt)       ||
                isa<DeclStmt>(currentStmt)       ||
                isa<IfStmt>(currentStmt)         ||
                isa<CompoundStmt>(currentStmt)   ||
                isa<ReturnStmt>(currentStmt)) {

                targetPoint = TargetProgramPoint(currentStmt, astContext);
                llvm::outs() << "TARGET FOUND: " << currentStmt->getStmtClassName()
                             << " at line " << sourceLineOfTargetStmt << "\n";
            }
        }
        return true;
    }
};
