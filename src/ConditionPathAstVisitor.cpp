#pragma once
#include "main.h"


class ConditionPathAstVisitor : public RecursiveASTVisitor<ConditionPathAstVisitor> {
public:
  TargetProgramPoint targetPoint;
 

private:
 

public:
  ASTContext &astContext;
  SourceManager &srcmgr;
  Rewriter &rewriter;
  
  explicit ConditionPathAstVisitor(SourceManager& _srcmgr, Rewriter& _rewriter,ASTContext& Ctx)
      :srcmgr(_srcmgr), rewriter(_rewriter),astContext(Ctx)// astContext(&(CI->getASTContext())) // initialize private members
  {
    
  }

  bool VisitStmt(Stmt *s) {
    visitStmtCall++;

    FullSourceLoc targetStatementLocation = astContext.getFullLoc(s->getBeginLoc());
    
    if (astContext.getSourceManager().isInMainFile(targetStatementLocation)) {
        if (targetStatementLocation.getSpellingLineNumber() == sourceLineFromArg) {
            
            // ✅ KEY FIX: Only capture statements that are "complete" statements
            // Filter out expressions that are just components of larger statements
            
            if (isa<BinaryOperator>(s) || 
                isa<CallExpr>(s) || 
                isa<DeclStmt>(s) || 
                isa<IfStmt>(s) || 
                isa<CompoundStmt>(s) ||
                isa<ReturnStmt>(s)) {
                
                // This is a "real" statement, not just a sub-expression
                globalScopeTargetStmt = s;
                targetPoint = TargetProgramPoint(s, astContext);
                
                llvm::outs() << "TARGET FOUND: " << s->getStmtClassName() 
                           << " at line " << sourceLineFromArg << "\n";
                
                // Don't return false here - we want to continue to find the best match
                // But we'll handle this differently below
            }
            // Don't return early - let traversal continue to find better matches
        }
    }
    return true;
}
/*  bool VisitStmt(Stmt *s) {
    visitStmtCall++;

    FullSourceLoc tragetStatementLocation =
        astContext.getFullLoc(s->getBeginLoc());
   
        
    if (astContext.getSourceManager().isInMainFile(tragetStatementLocation)) {
        
      // Expr* condition;
       //cout<<"currentLine "<<tragetStatementLocation.getSpellingLineNumber()<<"\n";    

      if (tragetStatementLocation.getSpellingLineNumber() ==
          sourceLineFromArg) {
        // globalTargetStmt = s;
        
        targetPoint = TargetProgramPoint(s, astContext);
        globalScopeTargetStmt=s;
        if(s){
          //s->dump();
        }else{
          cout<<"<<s NULL>>\n";
        }
        //return false to stop the analysis at this point 
        return true;

      }else {
        return true;
      }
    }
    
  }
*/
  /*

  bool VisitFunctionDecl(FunctionDecl *f) {
    string funcName = f->getNameInfo().getName().getAsString();
    //cout<<"visiting : "<< funcName<< " \n";
    if (funcName == functionNameDump) {
      Stmt *funcBody = f->getBody();
      f->dump();

      // std::unique_ptr<CFG> sourceCFG =          CFG::buildCFG(f, funcBody,
      // astContext, CFG::BuildOptions());
      // sourceCFG->print(llvm::errs(), LangOptions(), true);
      return false;
    }

    return true;
  }
  /*bool VisitDecl(Decl *D) {
    FullSourceLoc DeclNode = astContext->getFullLoc(D->getBeginLoc());
    if (astContext->getSourceManager().isInMainFile(DeclNode)) {

      // decl->dump();
      if (VarDecl *VD = dyn_cast<VarDecl>(D->getSingleDecl())) {
        // It's a reference to a variable (a local, function parameter,global,
        // or static data member).
        std::cout << "Variable name " << VD->getNameAsString()
                  << "\tVariable type " << (VD->getType()).getAsString()
                  << "\n";
        // D->dump();
      }
    }
  }*/
};
