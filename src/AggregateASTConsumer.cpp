#pragma once
#include "main.h"

class AggregateASTConsumer : public clang::ASTConsumer {
 
public:
    AggregateASTConsumer(SourceManager& srcmgr, Rewriter& rewriter,ASTContext& Ctx){

    }
    AggregateASTConsumer(){}

    void HandleTranslationUnit(clang::ASTContext& Ctx) override {
         for (auto consumer: consumers){
              cout<<"consumeeers : "<< consumer<<"\n";
              consumer->HandleTranslationUnit(Ctx);
          }

    }
    std::vector<ASTConsumer*> consumers;
};

