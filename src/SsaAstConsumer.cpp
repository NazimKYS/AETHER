#pragma once
#include "main.h"

class SsaASTConsumer : public ASTConsumer {
private:
    SsaAnalyzerAstVisitor visitor;
public:
 
    SsaASTConsumer(SourceManager& srcmgr, Rewriter& rewriter,ASTContext& Ctx ):
    visitor(srcmgr, rewriter,Ctx)
    {
    }
    
  


    void HandleTranslationUnit(ASTContext& context) override {
        visitor.TraverseDecl(context.getTranslationUnitDecl());

        // Output example
         llvm::outs() << "\n=== SSA Definitions by Line ===\n";
        for (const auto& [line, defs] : visitor.getLineMap()) {
            for (const auto& def : defs) {
                llvm::outs() << "Line " << line << ": " << def.exprString
                             << " (stmtID=" << def.stmtID << ")\n";
            }
        }

        llvm::outs() << "\n=== SSA Variable Version History ===\n";
        visitor.printVariableHistory();
    }


};
