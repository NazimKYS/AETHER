#pragma once
#include "main.h"




void extractUsedVariables(Expr *expr){

  if(isa<DeclRefExpr>(expr)){
    DeclRefExpr *dre = dyn_cast<DeclRefExpr>(expr);
    if (VarDecl *vd = dyn_cast<VarDecl>(dre->getDecl())) {
       
        TrackedVariables leftVar = TrackedVariables(
            vd->getNameAsString(), (vd->getType()).getAsString(),  vd);
            if(vd->getNameAsString()=="uid_sid"){targetVariableUidSid=leftVar;}
    
    }
  }else if(isa<ParenExpr>(expr)){
    ParenExpr *parenExpr= dyn_cast<ParenExpr>(expr);
    if(isa<BinaryOperator>(dyn_cast<BinaryOperator>(parenExpr->getSubExpr()))){
      BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
      //call extract
      extractUsedVariables(dyn_cast<Expr>(binOpStmt));
      //cout<<"  extractUsedVariables : " <<(dyn_cast<Stmt>(binOpStmt))->getStmtClassName()<<"\n";

    }
      
  }else if(isa<BinaryOperator>(expr)){
    BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(expr);
    Expr *lhs = (binOpStmt->getLHS())->IgnoreImpCasts();
      //cout<<" initialize rhs\n";
    Expr *rhs = (binOpStmt->getRHS())->IgnoreImpCasts();
    extractUsedVariables(lhs);
    extractUsedVariables(rhs);
    
      
  }else if(isa<ImplicitCastExpr>(expr)){
    Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
    extractUsedVariables(SkipingImplicitCast);
  
  }/*else if(isa<MemberExpr>(expr)){
    Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
    extractUsedVariables(SkipingImplicitCast);
  
  }*/else{
    //cout<<" extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";
    //expr->dump();
    //cout<<"not decl ref\n";
  }
}


void extractUsedVariables(const Expr *expr){

  if(dyn_cast<DeclRefExpr>(expr)){
    const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(expr);
    if (const VarDecl *vd = dyn_cast<VarDecl>(dre->getDecl())) {
       
        TrackedVariables leftVar = TrackedVariables(
            vd->getNameAsString(), (vd->getType()).getAsString(), vd);
    
    }
  }else if(dyn_cast<ParenExpr>(expr)){
    const ParenExpr *parenExpr= dyn_cast<ParenExpr>(expr);
    if(isa<BinaryOperator>(dyn_cast<BinaryOperator>(parenExpr->getSubExpr()))){
      const BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(parenExpr->getSubExpr());

      cout<<" const extractUsedVariables : " <<(dyn_cast<Stmt>(binOpStmt))->getStmtClassName()<<"\n";

    }
      



  }else{
    cout<<" const extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";

    //cout<<"not decl ref\n";
  }
}


void extractVariablesFromConditiont(const Stmt *stmt,
                                    ASTContext *astContext) {
  const SourceManager &srcMgr = astContext->getSourceManager();
 
  
    if(dyn_cast<BinaryOperator>(stmt)){
      const BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(stmt);


      Expr *lhs = (BinOpStmt->getLHS())->IgnoreImpCasts();
      Expr *rhs = (BinOpStmt->getRHS())->IgnoreImpCasts();

      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(lhs)) {
        extractUsedVariables(lhs);
    }
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(rhs)) {
        extractUsedVariables(rhs);
    }
  }
}

void trackVariables(Stmt *s) {

  if (isa<DeclStmt>(s)) {
    // cout<<tragetStatementLocation.getSpellingLineNumber()<< " : "<<
    // s->getStmtClassName()<<"\n";
    
  
    DeclStmt *decl = cast<DeclStmt>(s);
    if (VarDecl *VD = dyn_cast<VarDecl>(decl->getSingleDecl())) {
      // It's a reference to a variable (a local, function parameter, global,
      // or static data member).
      TrackedVariables var(VD->getNameAsString(), (VD->getType()).getAsString(),
                           VD);
      AllTrackedVariables.push_back(var);
      // std::cout << " varRef @ " << varRef << " : Variable name " <<
      // VD->getNameAsString() << "\t Variable type "<<
      // (VD->getType()).getAsString()<< "\n"; QualType type=VD->getType();
      // std::cout << "Variable type " << type.getAsString()<< "\n";
    } else {
      cout << "================== can't cast \n";
    }
    Stmt *currentSt = s;

    /*for (auto child : currentSt->children()){

        cout<<child->getStmtClassName()<<"\n";
        currentSt=currentSt->children();
    }*/
    /*for (Stmt::child_iterator child = s->child_begin(), e = s->child_end();
    child != e; ++child) {
        //Stmt *currStmt = *child;
        s->children()
        cout<<child->getStmtClassName()<<"\n";;
    }*/
  }
}

void trackDefOfVariables(BinaryOperator *BinOpStmt) {
  // const SourceManager &srcMgr = astContext->getSourceManager();
  if (BinOpStmt->isAssignmentOp()) {
    Expr *lhs = (BinOpStmt->getLHS())->IgnoreImpCasts();
    Expr *rhs = (BinOpStmt->getRHS())->IgnoreImpCasts();
    // def 
    llvm::outs() << BinOpStmt->getOpcodeStr() << "\n";
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(lhs)) {
      if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it != AllTrackedVariables.end()) {
          (it->vectorOfDefExpressions).push_back(rhs);
          // found element. it is an iterator to the first matching element.
          // if you really need the index, you can also get it:

        } else {
          
          cout << "not found==\n";
        }
        // It's a reference to a variable (a local, function parameter,
        // global, or static data member). llvm::outs()<< "{ "<<
        // (VD->getType()).getAsString()<<" } " << VD->getNameAsString() <<"
        // "<< BinOpStmt->getOpcodeStr()<<"\t";
      }
    }
  }
}
