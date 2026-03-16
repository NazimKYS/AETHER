#include "main.h"

class ConditionPathAstConsumer : public ASTConsumer {
private:
    ConditionPathAstVisitor visitor;

public:
    ConditionPathAstConsumer(SourceManager& srcmgr, Rewriter& rewriter, ASTContext& Ctx)
        : visitor(srcmgr, rewriter, Ctx) {}

    virtual void HandleTranslationUnit(ASTContext &Context) {
        auto startAst = high_resolution_clock::now();
        visitor.TraverseDecl(Context.getTranslationUnitDecl());
        auto endAst = high_resolution_clock::now();
        duration<double, std::milli> durationAst = endAst - startAst;

        if ((visitor.targetPoint).globalTargetStmt != NULL) {
            auto startParent = high_resolution_clock::now();
            (visitor.targetPoint).findParentStmt((visitor.targetPoint).globalTargetStmt);
            auto endParent = high_resolution_clock::now();

            auto startAnalysis = high_resolution_clock::now();
            (visitor.targetPoint).exploreFunctionByStmt();
            auto endAnalysis = high_resolution_clock::now();

            duration<double, std::milli> durationParent   = endParent   - startParent;
            duration<double, std::milli> durationAnalysis = endAnalysis - startAnalysis;

            cout << "\n\n ----- Durations -----\n";
            cout << " AST build    : " << durationAst.count()      << "ms\n";
            cout << " parent nodes : " << durationParent.count()   << "ms\n";
            cout << " analysis     : " << durationAnalysis.count() << "ms\n";
        } else {
            cout << " AST build: " << durationAst.count() << "ms\n";
            cout << "Can't find target stmt — check sourceline in target.json\n";
        }
    }
};
