#pragma once
#include "main.h"


struct SSAVariable {
    std::string name;
    int version;
    std::string ssaName() const { return name + "_" + std::to_string(version); }
};

// std::string rewriteWithSSA(const std::string& original, const std::vector<SSAVariable>& usedVars) {
//     std::string rewritten = original;
//     for (const auto& var : usedVars) {
//         size_t pos = 0;
//         while ((pos = rewritten.find(var.name, pos)) != std::string::npos) {
//             bool prefixOK = (pos == 0 || !isalnum(rewritten[pos - 1]));
//             bool suffixOK = (pos + var.name.size() >= rewritten.size() || !isalnum(rewritten[pos + var.name.size()]));
//             if (prefixOK && suffixOK) {
//                 rewritten.replace(pos, var.name.size(), var.ssaName());
//                 pos += var.ssaName().size();
//             } else {
//                 pos += var.name.size();
//             }
//         }
//     }
//     return rewritten;
// }


std::string rewriteWithSSA(const std::string& original, const std::vector<SSAVariable>& usedVars) {
    std::string rewritten = original;

    // Sort variables by length (longest first) to avoid partial matches
    std::vector<SSAVariable> sortedVars = usedVars;
    std::sort(sortedVars.begin(), sortedVars.end(),
        [](const SSAVariable& a, const SSAVariable& b) {
            return a.name.size() > b.name.size();
        });

    for (const auto& var : sortedVars) {
        const std::string& origName = var.name;
        const std::string& ssaName = var.ssaName();
        size_t pos = 0;
        while ((pos = rewritten.find(origName, pos)) != std::string::npos) {
            // Check left boundary: must be start or non-alnum
            bool leftOK = (pos == 0 || !isalnum(static_cast<unsigned char>(rewritten[pos - 1])));

            // Check right boundary: must be end or non-alnum
            size_t endPos = pos + origName.size();
            bool rightOK = (endPos >= rewritten.size() || 
                           !isalnum(static_cast<unsigned char>(rewritten[endPos])));

            // 🔑 CRITICAL: Also ensure it's not part of an existing SSA name like "userId_0"
            if (leftOK && rightOK) {
                // Look ahead: if followed by '_' and digit, it's already renamed → skip
                if (endPos < rewritten.size() && rewritten[endPos] == '_') {
                    size_t afterUnderscore = endPos + 1;
                    if (afterUnderscore < rewritten.size() && isdigit(static_cast<unsigned char>(rewritten[afterUnderscore]))) {
                        // This is likely an existing SSA name (e.g., userId_0), so skip
                        pos = endPos;
                        continue;
                    }
                }

                // Safe to replace
                rewritten.replace(pos, origName.size(), ssaName);
                pos += ssaName.size(); // move past the replacement
            } else {
                pos = endPos;
            }
        }
    }
    return rewritten;
}
struct DefinitionInfo {
    SSAVariable ssaVar;
    unsigned line;
    unsigned stmtID;
    std::string exprString;
    std::vector<SSAVariable> usedVars;
    std::vector<std::string> conditionContext;  // 🆕 added
};

class SSASymbolTable {
    std::unordered_map<std::string, int> currentVersion;
    std::unordered_map<std::string, std::vector<SSAVariable>> versionHistory;

public:
    SSAVariable define(const std::string& name) {
        int& version = currentVersion[name];
        SSAVariable ssaVar = {name, version++};
        versionHistory[name].push_back(ssaVar);
        return ssaVar;
    }

    SSAVariable latest(const std::string& name) const {
        auto it = versionHistory.find(name);
        if (it == versionHistory.end() || it->second.empty())
            return {name, 0}; // fallback
        return it->second.back();
    }

    const std::vector<SSAVariable>& history(const std::string& name) const {
        return versionHistory.at(name);
    }

    const std::unordered_map<std::string, std::vector<SSAVariable>>& getAll() const {
        return versionHistory;
    }
};

class SsaAnalyzerAstVisitor : public RecursiveASTVisitor<SsaAnalyzerAstVisitor> {
public:
    ASTContext &astContext;
    SourceManager &srcmgr;
    Rewriter &rewriter;
    
  
    SsaAnalyzerAstVisitor(SourceManager& _srcmgr, Rewriter& _rewriter,ASTContext& Ctx)
      :srcmgr(_srcmgr), rewriter(_rewriter),astContext(Ctx){
        cout << "SsaAnalyzerAstVisitor Constructor:  \n"  << endl;
    
    }


    bool VisitBinaryOperator(BinaryOperator* binOp) {
        if (binOp->isAssignmentOp()) {
            Expr* lhs = binOp->getLHS()->IgnoreParenImpCasts();
            Expr* rhs = binOp->getRHS()->IgnoreParenImpCasts();

            if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(lhs)) {
                std::string varName = declRef->getNameInfo().getAsString();
                SSAVariable newDef = symtab.define(varName);

                std::vector<SSAVariable> usedVars;
                collectUsedVars(rhs, usedVars);

                // Original RHS pretty-print
                std::string rawRHS;
                llvm::raw_string_ostream ss(rawRHS);
                rhs->printPretty(ss, nullptr, astContext.getPrintingPolicy());
                ss.flush();

                // Replace each raw variable in RHS with SSA version
                std::string substituted = ss.str();
                for (const auto& used : usedVars) {
                    std::string orig = used.name;
                    std::string versioned = used.ssaName();

                    // Replace all occurrences of orig with versioned
                    size_t pos = 0;
                    while ((pos = substituted.find(orig, pos)) != std::string::npos) {
                        bool leftOK = (pos == 0 || !isalnum(substituted[pos - 1]));
                        bool rightOK = (pos + orig.size() >= substituted.size() || !isalnum(substituted[pos + orig.size()]));
                        if (leftOK && rightOK) {
                            substituted.replace(pos, orig.length(), versioned);
                            pos += versioned.length();
                        } else {
                            pos += orig.length();
                        }
                    }
                }

                FullSourceLoc fullLoc = astContext.getFullLoc(binOp->getExprLoc());
                unsigned line = fullLoc.getSpellingLineNumber();
                unsigned stmtID = reinterpret_cast<uintptr_t>(binOp); // pointer-based ID
                std::stack<std::string> tmpStack = conditionStack;
                
                DefinitionInfo info = {
                    newDef,
                    line,
                    stmtID,
                    newDef.ssaName() + " = " + substituted,
                    usedVars,
                    std::vector<std::string>()  // copy stack to vector
                    //let try to copy full content of conditional stack here if its good ok else reset to previous statement
                    //tmpStack
                };
                while (!tmpStack.empty()) {
                    info.conditionContext.insert(info.conditionContext.begin(), tmpStack.top());
                    tmpStack.pop();
                }
                

                lineToDefinitions[line].push_back(info);
                varDefs[newDef.ssaName()] = info;
            }
        }
        return true;
    }
    
    bool VisitIfStmt(IfStmt* ifStmt) {
        Expr* condExpr = ifStmt->getCond();
        std::vector<SSAVariable> usedVars;
        collectUsedVars(condExpr, usedVars);

        std::string condText;
        llvm::raw_string_ostream ss(condText);
        condExpr->printPretty(ss, nullptr, astContext.getPrintingPolicy());
        std::string rewrittenCond = rewriteWithSSA(ss.str(), usedVars);

        SSAVariable condVar = symtab.define("cond"); // pseudo SSA condition var
        FullSourceLoc fullLoc = astContext.getFullLoc(ifStmt->getIfLoc());
        unsigned line = fullLoc.getSpellingLineNumber();
        unsigned stmtID = reinterpret_cast<uintptr_t>(ifStmt);

        DefinitionInfo condInfo = {
            condVar,
            line,
            stmtID,
            condVar.ssaName() + " = " + rewrittenCond,
            usedVars,
            {}  // no enclosing condition
        };
        lineToDefinitions[line].push_back(condInfo);
        varDefs[condVar.ssaName()] = condInfo;

        // Push current condition to stack
        conditionStack.push(rewrittenCond);

        // Visit then branch manually (to simulate correct scoping)
        TraverseStmt(ifStmt->getThen());

        // Handle else (with negated condition)
        if (Stmt* elseStmt = ifStmt->getElse()) {
            conditionStack.pop();
            conditionStack.push("!(" + rewrittenCond + ")");
            TraverseStmt(elseStmt);
        }

        conditionStack.pop(); // Pop after finishing if/else
        return false; // We already manually traversed children
    }


    void collectUsedVars(Expr* expr, std::vector<SSAVariable>& out) {
        if (!expr) return;

        if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(expr)) {
            std::string name = declRef->getNameInfo().getAsString();
            out.push_back(symtab.latest(name));
        }

        for (Stmt* child : expr->children()) {
            if (Expr* e = dyn_cast_or_null<Expr>(child)) {
                collectUsedVars(e, out);
            }
        }
    }

    const std::unordered_map<unsigned, std::vector<DefinitionInfo>>& getLineMap() const {
        return lineToDefinitions;
    }

    void printVariableHistory() const {
        std::unordered_map<std::string, std::vector<const DefinitionInfo*>> grouped;

        // Group versions by variable name (x_0, x_1 → x)
        for (const auto& [ssaName, def] : varDefs) {
            grouped[def.ssaVar.name].push_back(&def);
        }

        for (const auto& [var, defs] : grouped) {
            llvm::outs() << "Variable: " << var << "\n";
            for (const auto* def : defs) {
                llvm::outs() << "  [" << def->ssaVar.name << "] Line " << def->line
                             << ", stmtID=" << def->stmtID << ": " << def->exprString <<"\n";
                             // testing if conditional stack is empty to print full defintion contex
                if((def->conditionContext).size()==0){
                    llvm::outs() << " cond empty \n";
                }else{
                    for (size_t i = 0; i < (def->conditionContext).size(); i++)
                    {
                        cout<< (def->conditionContext)[i];
                        if(i==(def->conditionContext).size()-1){ 
                            cout << " \n";
                        }else{
                            cout <<" && \n";
                        }
                         /* code */
                    }
                    
                    
                }                 
            }
            
        }
    }

private:
    
    SSASymbolTable symtab;
    std::unordered_map<unsigned, std::vector<DefinitionInfo>> lineToDefinitions;
    std::unordered_map<std::string, DefinitionInfo> varDefs;
    std::stack<std::string> conditionStack;
};
