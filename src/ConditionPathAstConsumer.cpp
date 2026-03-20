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

            llvm::outs().flush();   // flush LLVM stream before launching Z3
            std::cout << std::flush;
            auto startZ3 = high_resolution_clock::now();
            int ret = std::system("python3 checking.py");
            if (ret != 0)
                cerr << "[!] python3 checking.py exited with code " << ret << "\n";
            auto endZ3 = high_resolution_clock::now();

            duration<double, std::milli> durationParent   = endParent   - startParent;
            duration<double, std::milli> durationAnalysis = endAnalysis - startAnalysis;
            duration<double, std::milli> durationZ3       = endZ3       - startZ3;

            cout << "\n\n ----- Durations -----\n";
            cout << " AST build    : " << durationAst.count()      << "ms\n";
            cout << " parent nodes : " << durationParent.count()   << "ms\n";
            cout << " script gen   : " << durationAnalysis.count() << "ms\n";
            cout << " Z3 checking  : " << durationZ3.count()       << "ms\n";
        } else {
            cout << " AST build: " << durationAst.count() << "ms\n";
            cout << "Can't find target stmt — check sourceline in target.json\n";
        }
    }
};
