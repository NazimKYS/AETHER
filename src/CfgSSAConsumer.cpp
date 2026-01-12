#include "main.h"
#include <optional>


// CfSSAConsumer.h
class CfSSAConsumer : public ASTConsumer {
    CfSSABuilder builder;
public:

CfSSAConsumer() {}
    void HandleTranslationUnit(ASTContext &Ctx) override {
        for (auto* D : Ctx.getTranslationUnitDecl()->decls()) {
            if (auto* FD = dyn_cast<FunctionDecl>(D)) {
                if (FD->getName() == "foo") { // or whatever target function
                    builder.buildSSAForFunction(FD, Ctx);
                    builder.dump(Ctx.getSourceManager());
                }
            }
        }
    }
};



void CfSSABuilder::buildSSAForFunction(const FunctionDecl* FD, ASTContext& Ctx) {
    if (!FD || !FD->getBody()) return;

    std::unique_ptr<CFG> cfg = CFG::buildCFG(
        FD, FD->getBody(), &Ctx,
        CFG::BuildOptions()
    );

    if (!cfg) return;
    processCFG(*cfg, Ctx);
}

void CfSSABuilder::processCFG(const clang::CFG& cfg, clang::ASTContext& Ctx) {
    std::vector<const clang::CFGBlock*> blocks;
    
    // Collect all real blocks
    for (auto i = cfg.begin(), e = cfg.end(); i != e; ++i) {
        const clang::CFGBlock* block = *i;
        if (block != &cfg.getEntry() && block != &cfg.getExit()) {
            blocks.push_back(block);
        }
    }

    // Sort by block ID → approximates source order
    std::sort(blocks.begin(), blocks.end(),
        [](const clang::CFGBlock* a, const clang::CFGBlock* b) {
            return a->getBlockID() < b->getBlockID();
        });

    VersionMap currentDefs;
    for (const clang::CFGBlock* block : blocks) {
        processBlock(*block, currentDefs, Ctx);
    }
}

void CfSSABuilder::processBlock(const clang::CFGBlock& block, VersionMap currentDefs, clang::ASTContext& Ctx) {
    for (const auto& elem : block) {
        std::optional<clang::CFGStmt> stmt = elem.getAs<clang::CFGStmt>();
        if (stmt) {
            if (const auto* BO = llvm::dyn_cast<clang::BinaryOperator>(stmt->getStmt())) {
                if (BO->isAssignmentOp()) {
                    handleAssignment(BO, currentDefs, Ctx);  // ← now const
                }
            }
        }
    }
}

void CfSSABuilder::handleAssignment(const clang::BinaryOperator* BO, VersionMap& currentDefs, clang::ASTContext& Ctx) {
    const clang::Expr* lhs = BO->getLHS()->IgnoreParenImpCasts();
    if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(lhs)) {
        if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
            std::string varName = VD->getNameAsString();

            unsigned& version = globalVersionCounter[varName];
            version++;

            SSAVar ssaVar;
            ssaVar.name = varName;
            ssaVar.version = version - 1;
            ssaVar.rhsExpr = BO->getRHS();  // ← STORE RHS
            ssaVar.defStmt = BO;
            ssaVar.loc = BO->getBeginLoc();
            ssaInfo[BO] = ssaVar;
            currentDefs[varName] = version - 1;
        }
    }
}


std::string CfSSABuilder::getVarName(Expr* expr) {
    if (auto* DRE = dyn_cast<DeclRefExpr>(expr->IgnoreParenImpCasts())) {
        if (auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
            return VD->getNameAsString();
        }
    }
    return "";
}

void CfSSABuilder::dump(const clang::SourceManager& SM) const {
    llvm::outs() << "\n=== CFG-Based SSA Definitions ===\n";
    // Sort by source location for readability
    std::vector<std::pair<SourceLocation, const SSAVar*>> sorted;
    for (const auto& [stmt, var] : ssaInfo) {
        sorted.emplace_back(var.loc, &var);
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });

    for (const auto& [loc, var] : sorted) {
        if (loc.isValid()) {
            unsigned line = SM.getSpellingLineNumber(loc);
            llvm::outs() << "Line " << line
                         << ": " << var->name << "_" << var->version << " = ";

            // Print RHS if available
            if (var->rhsExpr) {
                // Simple: print raw tokens
                auto range = clang::CharSourceRange::getTokenRange(var->rhsExpr->getSourceRange());
                auto str = clang::Lexer::getSourceText(range, SM, clang::LangOptions{});
                if (!str.empty()) {
                    llvm::outs() << str;
                } else {
                    llvm::outs() << "(expr)";
                }
            } else {
                llvm::outs() << "(no RHS)";
            }

            llvm::outs() << " (stmtID=" << var->defStmt << ")\n";
        }
    }
}