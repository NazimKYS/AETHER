#pragma once
#include "main.h"

class BuildIteration{
public:
    //static int round=0;
    Stmt* loopStmt;

    BuildIteration(){
        
    }

};


class Pass1Visitor : public RecursiveASTVisitor<Pass1Visitor>
{
private:
    ASTContext& astContext;
    string fileName;
    SourceManager &srcmgr;
    Rewriter &rewriter;
public:
    TargetProgramPoint targetPoint;
   

    Pass1Visitor(SourceManager& _srcmgr, Rewriter& _rewriter,ASTContext& Ctx )
      :srcmgr(_srcmgr), rewriter(_rewriter),astContext(Ctx)
    {
      cout << "Pass1Visitor Constructor: "  << endl;
    }

    ~Pass1Visitor() {
      //cout << "MyASTVisitor Deconstructor: "  << endl;
    }

    bool VisitStmt(Stmt *st) {
   
    visitStmtCall++;
    if(dyn_cast<ForStmt>(st)){
       
       ForStmt *loop=dyn_cast<ForStmt>(st);
       (loop->getBody ())->dump() ;
       //rewriter.insert

    }
    if(dyn_cast<WhileStmt>(st)){
       
        WhileStmt *loop=dyn_cast<WhileStmt>(st);
        (loop->getBody ())->dump() ;
       // get body end location

        FullSourceLoc startLocation = astContext.getFullLoc(loop->getBeginLoc());
        SourceRange sr = (loop->getBody ())->getSourceRange();
        cout<<std::string(get_source_text(sr, srcmgr)) << " \n";
        //cout<<std::string(get_source_text(sr, srcmgr)) << " \n";
        //cout<<std::string(get_source_text(sr, srcmgr)) << " \n";
    }
    
      return true;
    }
 

  bool VisitFunctionDecl(FunctionDecl *f) {
    ASTContext *astCtx=&astContext;
    string funcName = f->getNameInfo().getName().getAsString();
    //cout<<"visiting : "<< funcName<< " \n";
    if (funcName == functionNameDump) {
      Stmt *funcBody = f->getBody();
      //const std::unique_ptr<CFG> sourceCFG =CFG::buildCFG(f, funcBody,astCtx, CFG::BuildOptions());
      //CM=  CFGStmtMap::Build	(	sourceCFG.get(),PM );	
     

      exploreChildren(funcBody,0);
     
           
      return true;
    }

    return true;
  }
  
};


class Pass1Consumer : public ASTConsumer
{
private:
    Pass1Visitor Visitor;
public:
    Pass1Consumer(SourceManager& srcmgr, Rewriter& rewriter,ASTContext& Ctx)
        : Visitor(srcmgr, rewriter,Ctx) //initialize MyASTVisitor
    {}

    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
            // Travel each function declaration using MyASTVisitor
            Visitor.TraverseDecl(*b);
            
         
        }
        return true;
    }
     virtual void HandleTranslationUnit(ASTContext &Context) {

        auto stratChronoAstBuild = high_resolution_clock::now();
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        auto endChronoAstBuild = high_resolution_clock::now();
        duration<double, std::milli> msDoubleDurationAstBuild = endChronoAstBuild - stratChronoAstBuild;
        //std::cout <<" finding target statement in AST : " << msDoubleDurationAstBuild.count() << "ms  |  ";


    
    }


};




