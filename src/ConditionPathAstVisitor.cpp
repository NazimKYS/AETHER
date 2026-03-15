#pragma once
#include "main.h"


class ConditionPathAstVisitor : public RecursiveASTVisitor<ConditionPathAstVisitor> {
public:
  TargetProgramPoint targetPoint;
 
public:
  ASTContext &astContext;
  SourceManager &srcmgr;
  Rewriter &rewriter;
  
  explicit ConditionPathAstVisitor(SourceManager& _srcmgr, Rewriter& _rewriter,ASTContext& Ctx)
      :srcmgr(_srcmgr), rewriter(_rewriter),astContext(Ctx)// astContext(&(CI->getASTContext())) // initialize private members
  {
    
  }

  bool VisitStmt(Stmt *currentStmt) {
    visitStmtCall++;

    FullSourceLoc currentStmtLocation = astContext.getFullLoc(currentStmt->getBeginLoc());
    
    if (astContext.getSourceManager().isInMainFile(currentStmtLocation)) {
        if (currentStmtLocation.getSpellingLineNumber() == sourceLineOfTargetStmt) {
            
            // ✅ KEY FIX: Only capture statements that are "complete" statements
            // Filter out expressions that are just components of larger statements
            
            if (isa<BinaryOperator>(currentStmt) || 
                isa<CallExpr>(currentStmt) || 
                isa<DeclStmt>(currentStmt) || 
                isa<IfStmt>(currentStmt) || 
                isa<CompoundStmt>(currentStmt) ||
                isa<ReturnStmt>(currentStmt)) {
                
                // This is a "real" statement, not just a sub-expression
                globalScopeTargetStmt = currentStmt;
                targetPoint = TargetProgramPoint(currentStmt, astContext);
                
                llvm::outs() << "TARGET FOUND: " << currentStmt->getStmtClassName() 
                           << " at line " << sourceLineOfTargetStmt << "\n";
                
                // Don't return false here - we want to continue to find the best match
                // But we'll handle this differently below
            }
            // Don't return early - let traversal continue to find better matches
        }
    }
    return true;
}

};
