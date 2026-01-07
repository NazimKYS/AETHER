#pragma once
#include "main.h"


class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(SourceManager& srcmgr, Rewriter& rewriter,ASTContext& Ctx)
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
        currentCallCounter++;
        logFile<<"HandleTranslationUnit of MyASTConsumer with CALL COUNTER :# "<< currentCallCounter<<"\n";
        auto stratChronoAstBuild = high_resolution_clock::now();
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        auto endChronoAstBuild = high_resolution_clock::now();
        duration<double, std::milli> msDoubleDurationAstBuild = endChronoAstBuild - stratChronoAstBuild;
        std::cout <<" finding target statement in AST : " << msDoubleDurationAstBuild.count() << "ms  |  ";


    
    }

private:
    MyASTVisitor Visitor;
};
