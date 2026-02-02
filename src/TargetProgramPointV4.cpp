#pragma once
#include "main.h"

int getCurrentSsaVersionOfVariable(string baseName);

// Replace your current SSAVariable struct with this
struct SSAVariable {
    std::string name;        // Keep this as "name" to match your existing code
    int version;
    
    // NEW: Add definition info fields
    std::string exprString;
    const clang::Expr* definingExpr = nullptr;
    std::vector<std::string> pathConditions;
    
    std::string ssaName() const { 
        return name + "_" + std::to_string(version); 
    }
    
    //need to create default constructor for cond variables
    // Add default constructor (required for unordered_map)
    SSAVariable(const std::string& n) : name(n) {
        int currentVerionsIfexists = getCurrentSsaVersionOfVariable(n);
        version=currentVerionsIfexists+1;
    }
    SSAVariable() : version(0) {}
    // Constructor for existing code
    SSAVariable(const std::string& n, int v) : name(n), version(v) {}
    
    // Constructor with definition info
    SSAVariable(const std::string& n, int v, const std::string& expr, 
                const clang::Expr* defExpr = nullptr) 
        : name(n), version(v), exprString(expr), definingExpr(defExpr) {}
    
    SSAVariable(const std::string& varName,
                const std::string& exprStr,  // Renamed to avoid shadowing
                const clang::Expr* defExpr = nullptr,
                const std::vector<std::string>& pathConds = {})
        : name(varName),
          version(getCurrentSsaVersionOfVariable(varName) + 1),
          exprString(exprStr),      // No shadowing ambiguity
          definingExpr(defExpr),
          pathConditions(pathConds)
    {
        // Constructor body can stay empty (all members initialized above)
    }
};


// Structure to hold definition info
struct SSAVarInfo {
    bool found = false;
    std::string baseName;
    int version = -1;
    std::string exprString;
    const clang::Expr* definingExpr = nullptr;
    std::vector<std::string> pathConditions;
};


// Get definition by SSA name (e.g., "serviceId_3")

// SMT Context structures
struct SMTVariableDefinition {
    std::string baseName;
    std::string ssaName;
    int version;
    const clang::Expr* definingExpr = nullptr;
    const clang::Stmt* defStmt = nullptr;
    std::string exprString;
    std::vector<std::string> pathConditions;
    unsigned line;
    unsigned stmtID;
};

struct SMTTargetContext {
    const clang::Stmt* targetStmt = nullptr;
    std::vector<std::string> targetPathConditions;
    std::vector<SSAVariable> variablesUsedIn;
    std::unordered_map<std::string, std::vector<SMTVariableDefinition>> variableDefinitions;
    std::vector<std::string> targetVariables;
    std::set<std::string> relevantVariables;
};

struct DefinitionInfo {
    SSAVariable ssaVar;
    unsigned line;
    unsigned stmtID;
    std::string exprString;
    std::vector<std::string> usedVars;
    std::vector<std::string> conditionContext;
    
    // NEW: Store original AST nodes for SMT generation
    const clang::Expr* definingExpr = nullptr;    // RHS of assignment
    const clang::Expr* conditionExpr = nullptr;   // For if conditions
    const clang::Stmt* defStmt = nullptr;        // The statement itself
};


//make it global
std::unordered_map<std::string, DefinitionInfo> varDefs;

int getCurrentSsaVersionOfVariable(string baseName){
    int currentVersion=-1;
    std::string latestSSA = "";
        
        for (const auto& [ssaName, def] : varDefs) {
            if (def.ssaVar.name == baseName) {
                if (def.ssaVar.version > currentVersion) {
                    currentVersion = def.ssaVar.version;
                    latestSSA = ssaName;
                }
            }
        }
        return currentVersion;
}



std::string rewriteWithSSA(const std::string& original, const std::vector<std::string>& usedVars) {
    std::string rewritten = original;
    std::vector<SSAVariable> sortedVars ={};
    // Sort variables by length (longest first) to avoid partial matches
    // getSSAvariable from symbol table using ssaVarName:
    for (const auto& var : usedVars) {
        auto it = varDefs.find(var);
        if (it != varDefs.end()) {
            const DefinitionInfo& def = it->second;
            sortedVars.push_back(def.ssaVar);
        }

    
    }    
      
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



// Helper functions (implement these as shown in previous answer)

class TargetProgramPoint {
    
private:
    //SSASymbolTable symtab;
    std::unordered_map<unsigned, std::vector<DefinitionInfo>> lineToDefinitions;
    // make it global // std::unordered_map<std::string, DefinitionInfo> varDefs;
    std::stack<std::string> conditionStack;  
public:
    const Stmt *globalTargetStmt;
    ASTContext *globalAstContext;
    std::vector<StmtAttributes> vectorOfParentStmt = {};
    const FunctionDecl *parentFunctionOfProgramPoint;
    std::vector<std::unordered_map<std::string, SSAVariable>> ssaStateStack;
    std::set<std::string> variablesRelatedToTargetStmt;
    // Add to your class:
    bool targetFound = false;
    std::vector<std::string> getCurrentPathConditions() const;

struct PathCondition {
    const clang::Expr* conditionExpr = nullptr;  // Initialize pointer!
    bool isTrueBranch = false;                   // Default to else branch
    std::string smtString;                       // Empty by default (std::string is safe)
    std::vector<std::string> ssaUsedVars;

    // Default constructor (optional but safe)
    PathCondition() = default;

    // CORRECT constructor using member initializer list
    PathCondition(const clang::Expr* condExpr, 
                  bool trueBranch,
                  std::vector<std::string> usedVars = {})
        : conditionExpr(condExpr),
          isTrueBranch(trueBranch),
          ssaUsedVars(std::move(usedVars))  // Efficient move
    {}
    void printUsedVars() const {
        std::cout << "SSA vars (" << ssaUsedVars.size() << "): ";
        for (size_t i = 0; i < ssaUsedVars.size(); ++i) {
            std::cout << ssaUsedVars[i];
            if (i < ssaUsedVars.size() - 1) std::cout << ", ";
        }
        std::cout << '\n';
    }

};

     std::vector<PathCondition> pathConditions; // ← NEW
     std::stack<PathCondition> gloablPathConditions; // ← NEW

    TargetProgramPoint(){};
    TargetProgramPoint(const Stmt *targetStmt, ASTContext &astContext) {
        globalTargetStmt = targetStmt;
        globalAstContext = &astContext;
        sharedASTContext = &astContext;
        //cout << "from target " << globalTargetStmt->getID(*globalAstContext)<< "\n";
        globalTargetStmt->dump();
    }

    void findParentStmt(const Stmt *s, bool allParentsFound) {
        if (allParentsFound) {
            cout << "all parent found : " << vectorOfParentStmt.size() << "\n";
        } else {
            const Stmt *ST = s;
            const auto &parents = globalAstContext->getParents(*ST);  // ← only call once

            if (parents.empty()) {
            llvm::errs() << "Can not find parent\n";
            allParentsFound = true;
            } else {
            // FIXED: use 'parents', not a new temporary!
            auto it = parents.begin();
            llvm::outs() << "Current Node Kind: " << it->getNodeKind().asStringRef() << "\n";

            if (it->getNodeKind().asStringRef() == "FunctionDecl") {
                parentFunctionOfProgramPoint = parents[0].get<FunctionDecl>();
            }

            if (parents.size() == 1) {
                ST = parents[0].get<Stmt>();
                if (!ST) {
                allParentsFound = true;
                findParentStmt(ST, allParentsFound);
                } else {
                StmtAttributes parentStmt =
                    StmtAttributes(ST->getID(*globalAstContext), ST);
                vectorOfParentStmt.push_back(parentStmt);
                findParentStmt(ST, allParentsFound);
                }
            }
            }
        }
}

    void getConditionPathV2() {
    cout<<"running getConditionPathV2 here\n";
    ASTContext *astContext = globalAstContext;

    const SourceManager &srcMgr = astContext->getSourceManager();

    const Expr *condition;
    std::string expression = "";
    std::vector<std::string> allExpressions = {};

    std::string condState = "";
    //pathConditions.clear();
    for (unsigned int i = 0; i < vectorOfParentStmt.size(); ++i) {
      //cout<<" for loop getConditionPath index "<<i<<" \n";
      const Stmt *st = vectorOfParentStmt[i].st;
      if (isa<IfStmt>(st)) {
        const IfStmt *ifStmt = cast<IfStmt>(st);
        condition = ifStmt->getCond();
        condition->dump();
        if (!condition) continue;

        bool isTrueBranch = false;
        //checking condition state disable comment after mapping new nodes of dataStructure
        
        if ( vectorOfParentStmt.size()> (i - 1) ) {
          StmtAttributes nextStmt = vectorOfParentStmt[i - 1];
          if (nextStmt.id == (ifStmt->getThen())->getID(*globalAstContext)) {
            // comment line 260 & 266
            //cout<<"vectorSize = "<<vectorOfParentStmt.size()<<" access element vector[ "<<i - 1 <<" ]\n";
            condState = "";
            isTrueBranch = true;
            //listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));

          }
          if(ifStmt->getElse()){
            if(nextStmt.id == (ifStmt->getElse())->getID(*globalAstContext)) {
                condState = "Not";
                isTrueBranch = false;
                //listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not"));
            }
          }
          
        } // check if we are in the block of then or in the else block
       
        std::vector<std::string> vars;
        collectUsedVarsFromCondPath(const_cast<clang::Expr*>(condition), vars);  // ✅ Called on 'this' instance

        // Now construct with precomputed data
        //PathCondition pc(condition, isTrueBranch, vars);
       
        // pc.conditionExpr = condition;
        // pc.isTrueBranch = isTrueBranch;

        const Stmt *conditionStmt= ifStmt->getCond(); 
        if(condState=="Not"){
          
          NodeTool *tree = new NodeTool(conditionStmt);
          NodeTool root(condState,tree);
          //cout<<"roooooooooooooot Not \n"<<root.pyz3ApiFlatten()<<"\n";
          //pc.smtString = root.pyz3ApiFlatten();
        }else{
          //NodeTool *tree = new NodeTool(conditionStmt); /* to avoid ==10661==ERROR: LeakSanitizer: detected memory leaks Direct leak of 96 byte(s) in 1 object(s) allocated from:    1 0x5d81cfdda85a in TargetProgramPoint::getConditionPathV2() src/TargetProgramPoint.cpp:315 */
          NodeTool tree(conditionStmt);
          //cout<<"roooooooooooooot true \n"<<tree.pyz3ApiFlatten()<<"\n";
          //pc.smtString = tree.pyz3ApiFlatten();
          
        }
        //pathConditions.push_back(pc);
        //pc.printUsedVars();
        binaryOpToStrV2(conditionStmt,globalAstContext);
        
        // testing binaryStrV2 
        /*
        allExpressions.push_back(
            condState + "( " +  binaryOpToStr(conditionStmt, srcMgr, globalAstContext)  + " )");
        //cout<<"allexpression size"<< allExpressions.size()<<"\n";*/
      }  
    }

  }
  

    bool VisitBinaryOperator(BinaryOperator* binOp, const Stmt* originalStmt = nullptr) {
    if (binOp->isAssignmentOp()) {
        Expr* lhs = binOp->getLHS()->IgnoreParenImpCasts();
        Expr* rhs = binOp->getRHS()->IgnoreParenImpCasts();

        if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(lhs)) {
            std::string varName = declRef->getNameInfo().getAsString();

            // Collect used variables (you'll need to pass current path state)
            std::vector<string> usedVars;
            collectUsedVars(rhs, usedVars); // heere we have  Update this later if needed
            llvm::outs() << "DEBUG: For " << varName << ", usedVars: ";
            for (const auto& used : usedVars) {
                llvm::outs() << used << " ";
            }
            llvm::outs() << "\n";
            // Create substituted RHS string
            std::string rawRHS;
            llvm::raw_string_ostream ss(rawRHS);
            rhs->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy()); //str definition
            ss.flush();

            std::string substituted = ss.str();
            //substitute varibales names with their ssa version in definition expression, can be optimized
            std::vector<SSAVariable> usedSsaVars ={};
            // Sort variables by length (longest first) to avoid partial matches
            // getSSAvariable from symbol table using ssaVarName:
            for (const auto& var : usedVars) {
                auto it = varDefs.find(var);
                if (it != varDefs.end()) {
                    const DefinitionInfo& def = it->second;
                    usedSsaVars.push_back(def.ssaVar);
                }

            
            } 
            for (const auto& used : usedSsaVars) {
                std::string orig = used.name; // Use baseName now
                std::string versioned = used.ssaName();
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

            // Get current path conditions
            std::vector<std::string> currentPathConditions;
            std::stack<std::string> tmpStack = conditionStack;
            while (!tmpStack.empty()) {
                currentPathConditions.insert(currentPathConditions.begin(), tmpStack.top());
                tmpStack.pop();
            }

            // ✅ USE ENHANCED SYMBOL TABLE
            /*SSAVariable newDef = symtab.defineWithDefinition(
                varName, 
                substituted, 
                rhs, 
                currentPathConditions
            );*/
            SSAVariable newDef(
                varName, 
                substituted, 
                rhs, 
                currentPathConditions
            );

            // Rest of your code remains the same...
            FullSourceLoc fullLoc = globalAstContext->getFullLoc(binOp->getExprLoc());
            unsigned line = fullLoc.getSpellingLineNumber();
            unsigned stmtID = reinterpret_cast<uintptr_t>(binOp);

            // You can still store in varDefs if needed for debugging
            DefinitionInfo info = {
                newDef,
                line,
                stmtID,
                newDef.ssaName() + " = " + substituted,
                usedVars,
                currentPathConditions,
                rhs,
                nullptr,
                originalStmt ? originalStmt : binOp
            };
            lineToDefinitions[line].push_back(info);
            varDefs[newDef.ssaName()] = info;
            

        }
    }
    return true;
}
  
    void collectUsedVars(Expr* expr, std::vector<std::string>& out) {
        if (!expr) return;

        if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(expr)) {
            std::string name = declRef->getNameInfo().getAsString();
            //out.push_back(symtab.latest(name)); //checking if findLatestSSAName get the same result
            out.push_back(findLatestSSAName(name)); 
           
        }
        
        for (Stmt* child : expr->children()) {
            if (Expr* e = dyn_cast_or_null<Expr>(child)) {
                collectUsedVars(e, out);
            }
        }
    }

    void collectUsedVarsFromCondPath(Expr* expr, std::vector<std::string>& out) {

        
        if(isa<DeclRefExpr>(expr)){
            DeclRefExpr *dre = dyn_cast<DeclRefExpr>(expr);
            if (VarDecl *VD = dyn_cast<VarDecl>(dre->getDecl())) {
                std::string name = dre->getNameInfo().getAsString();
                cout<< "&&&& collectUsedVarsFromCondPath current name : "<<name <<"\n";
                out.push_back(findLatestSSAName(name));

            }
        }else if(isa<ParenExpr>(expr)){
            ParenExpr *parenExpr= dyn_cast<ParenExpr>(expr);
            if(isa<BinaryOperator>(dyn_cast<BinaryOperator>(parenExpr->getSubExpr()))){
                BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
                collectUsedVarsFromCondPath(dyn_cast<Expr>(binOpStmt),out);
            }
            
        }else if(isa<BinaryOperator>(expr)){
            BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(expr);
            Expr *lhs = (binOpStmt->getLHS())->IgnoreImpCasts();
            Expr *rhs = (binOpStmt->getRHS())->IgnoreImpCasts();
            collectUsedVarsFromCondPath(rhs,out);
            collectUsedVarsFromCondPath(lhs,out);
            
            
        }else if(isa<ImplicitCastExpr>(expr)){
            Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
            collectUsedVarsFromCondPath(SkipingImplicitCast,out);
        
        }else if(isa<CStyleCastExpr>(expr)){
            Expr *subExpr= (dyn_cast<CStyleCastExpr>(expr))->getSubExpr();
            collectUsedVarsFromCondPath(subExpr,out);
        
        }else if(isa<MemberExpr>(expr)){
            Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
            collectUsedVarsFromCondPath(SkipingImplicitCast,out);
        
        }else{/*
            logFile<<" extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";
            cout<<" extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";
            //expr->dump();
            cout<<"not decl ref\n";*/
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

 

    void exploreFunctionByStmt() {
        const FunctionDecl* func = parentFunctionOfProgramPoint;
        if (!func || !func->hasBody()) {
            return;
        }
        
        const Stmt* body = func->getBody();
        //body->dump();
        // Reset state before traversal
        targetFound = false;
        
        // Perform traversal
        bool found = visitAllStmts(body);
        
        if (found) {
            llvm::outs() << "\n=== Building SMT Context ===\n";
            SMTTargetContext smtContext = buildSMTContext();
            printSMTContext(smtContext);
            llvm::outs() << "\n=== Detailed Variable Information ===\n";
            for (const auto& ssaName : smtContext.relevantVariables) {
                if (!ssaName.empty()) {
                    llvm::outs() << "Variable: " << ssaName << "\n";
                    
                    SSAVarInfo info = getSSAVarInfoFromVarDefs(ssaName);
                    if (info.found) {
                        llvm::outs() << "  Base name: " << info.baseName << "\n";
                        llvm::outs() << "  Version: " << info.version << "\n";
                        llvm::outs() << "  Expression: " << info.exprString << "\n";
                        
                        // Optional: print path conditions
                        if (!info.pathConditions.empty()) {
                            llvm::outs() << "  Path conditions:\n";
                            for (const auto& cond : info.pathConditions) {
                                llvm::outs() << "    " << cond << "\n";
                            }
                        }
                    } else {
                        llvm::outs() << "  NOT FOUND in varDefs\n";
                    }
                    llvm::outs() << "\n";
                }
            }

        } else {
            llvm::errs() << "Target statement not found!\n";
        } 
        //cout<< "!!! printing variable history \n";
        //printVariableHistory();


    }


    bool visitAllStmts(const Stmt* stmt) {
        if (!stmt) return false;

        if (stmt == globalTargetStmt) {
            if (isa<BinaryOperator>(stmt)) {
                BinaryOperator* binOp = const_cast<BinaryOperator*>(cast<BinaryOperator>(stmt));
                VisitBinaryOperator(binOp, stmt);
            }
            // Add other statement types as needed (CallExpr, etc.)
            
            llvm::outs() << "******* TARGET STMT FOUND — STOPPING \n";
            printConditionStackTopToBottom(conditionStack);
            targetFound = true;
            return true;
        }

        if (isa<IfStmt>(stmt)) {
            IfStmt* ifStmt = const_cast<IfStmt*>(cast<IfStmt>(stmt));
            Expr* condExpr = ifStmt->getCond();
            
            std::vector<string> usedVars;
            collectUsedVars(condExpr, usedVars); //extract variables used from condExpression in list usedVars

            std::string condText;
            llvm::raw_string_ostream ss(condText);
            condExpr->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy());
            std::string rewrittenCond = rewriteWithSSA(ss.str(), usedVars);

            SSAVariable condVar("cond");//symtab.define("cond");
            FullSourceLoc fullLoc = globalAstContext->getFullLoc(ifStmt->getIfLoc());
            unsigned line = fullLoc.getSpellingLineNumber();
            unsigned stmtID = reinterpret_cast<uintptr_t>(ifStmt);

            DefinitionInfo condInfo = {
                condVar,
                line,
                stmtID,
                condVar.ssaName() + " = " + rewrittenCond,
                usedVars,
                {},  // conditionContext will be filled from stack
                nullptr,  // definingExpr
                condExpr, // conditionExpr
                ifStmt    // defStmt
            };
            
            // Fill condition context from current stack
            std::stack<std::string> tmpStack = conditionStack;
            while (!tmpStack.empty()) {
                condInfo.conditionContext.insert(condInfo.conditionContext.begin(), tmpStack.top());
                tmpStack.pop();
            }

            lineToDefinitions[line].push_back(condInfo);
            varDefs[condVar.ssaName()] = condInfo;

            std::vector<std::string> vars;
            collectUsedVarsFromCondPath(condExpr, vars);  // ✅ Called on 'this' instance

            // Now construct with precomputed data
            
        
            
             

            if (const Stmt* thenBody = ifStmt->getThen()) {
                conditionStack.push(rewrittenCond);
                PathCondition pc(condExpr, true, vars);
                pc.smtString=rewrittenCond;
                pc.printUsedVars();
                gloablPathConditions.push(pc);
                if (visitAllStmts(thenBody)) return true;
                conditionStack.pop();
                gloablPathConditions.pop();
            }

            if (const Stmt* elseBody = ifStmt->getElse()) {
                conditionStack.push("!(" + rewrittenCond + ")");
                PathCondition pc(condExpr, false, vars);
                pc.smtString="!(" + rewrittenCond + ")";
                 pc.printUsedVars();
                gloablPathConditions.push(pc);
                if (visitAllStmts(elseBody)) return true;
                conditionStack.pop();
                gloablPathConditions.pop();
            }
            return false;
        }

        if (isa<BinaryOperator>(stmt)) {
            BinaryOperator* binOp = const_cast<BinaryOperator*>(cast<BinaryOperator>(stmt));
            VisitBinaryOperator(binOp, stmt); // Pass stmt for AST preservation
        }

        for (auto child = stmt->child_begin(); child != stmt->child_end(); ++child) {
            if (visitAllStmts(*child)) {
                return true;
            }
        }

        return false;
    }

    void printPathConditions() {
        if (pathConditions.empty()) {
            llvm::outs() << "No path conditions.\n";
            return;
        }

        
        for (size_t i = 0; i < pathConditions.size(); ++i) {
            const auto& pc = pathConditions[i];
            llvm::outs()  << pc.smtString <<  "\n";
            /*if(pc.isTrueBranch){ 
            llvm::outs()  << "(" << pc.smtString << ") \n";
            }else{
            llvm::outs()  << "Not(" << pc.smtString << ") \n";
            }*/
            
        }
    } 




  void printConditionStackTopToBottom(std::stack<std::string>& stack) {
    if (stack.empty()) {
        llvm::outs() << "Condition stack: [empty]\n";
        return;
    }

    std::stack<std::string> temp = stack;
    llvm::outs() << "Current path conditions (most recent first):\n";
    while (!temp.empty()) {
        llvm::outs() << "  " << temp.top() << "\n";
        temp.pop();
    }
  }

private:
    // SMT context building methods

    void extractVariablesFromExpr(const clang::Expr* expr, std::vector<std::string>& vars) const {

       if (!expr) return;
    
    if (const auto* dre = dyn_cast<clang::DeclRefExpr>(expr)) {
        std::string name = dre->getNameInfo().getAsString();
        if (!name.empty()) {
            // Extract base name from SSA name if needed
            size_t lastUnderscore = name.find_last_of('_');
            if (lastUnderscore != std::string::npos && lastUnderscore > 0) {
                std::string suffix = name.substr(lastUnderscore + 1);
                if (!suffix.empty() && std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
                    name = name.substr(0, lastUnderscore);
                }
            }
            vars.push_back(name);
        }
        return;
    }
    
    // Handle binary operators (like multiplication)
    if (const auto* binOp = dyn_cast<clang::BinaryOperator>(expr)) {
        extractVariablesFromExpr(binOp->getLHS(), vars);
        extractVariablesFromExpr(binOp->getRHS(), vars);
        return;
    }
    
    // Handle casts
    if (const auto* ice = dyn_cast<clang::ImplicitCastExpr>(expr)) {
        extractVariablesFromExpr(ice->getSubExpr(), vars);
        return;
    }
    
    // Handle other common expressions
    for (const auto* child : expr->children()) {
        if (const auto* childExpr = dyn_cast_or_null<clang::Expr>(child)) {
            extractVariablesFromExpr(childExpr, vars);
        }
    }
}


    std::vector<std::string> extractVariablesFromString(const std::string& str) const {
    std::vector<std::string> vars;
    if (str.empty()) return vars;
    
    // Simple tokenization
    std::string current;
    for (char c : str) {
        if (std::isalnum(c) || c == '_') {
            current += c;
        } else {
            if (!current.empty()) {
                // Skip if it starts with a digit (likely a number)
                if (!std::isdigit(current[0])) {
                    vars.push_back(current);
                }
                current.clear();
            }
        }
    }
    if (!current.empty() && !std::isdigit(current[0])) {
        vars.push_back(current);
    }
    
    // Remove SSA version suffixes (_0, _1, _2, etc.) to get base names
    std::vector<std::string> baseNames;
    for (auto& var : vars) {
        size_t pos = var.find('_');
        if (pos != std::string::npos && pos > 0) {
            std::string suffix = var.substr(pos + 1);
            if (!suffix.empty() && std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
                // This is an SSA variable - extract base name
                baseNames.push_back(var.substr(0, pos));
            } else {
                // Not an SSA suffix, keep as-is
                baseNames.push_back(var);
            }
        } else {
            // No underscore, keep as-is
            baseNames.push_back(var);
        }
    }
    
    return baseNames;
}


    std::set<std::string> computeRelevantVariablesWithSSANames() const {
        std::set<std::string> relevantSSANames;
        
        // STEP 1: Extract variables from target statement and get their current SSA versions
        if (const auto* binOp = dyn_cast<clang::BinaryOperator>(globalTargetStmt)) {
            if (binOp->isAssignmentOp()) {
                // Get LHS variable
                if (const auto* lhs = dyn_cast<clang::DeclRefExpr>(binOp->getLHS())) {
                    std::string targetBaseName = lhs->getNameInfo().getAsString();
                    std::string targetSSA = findLatestSSAName(targetBaseName);
                    if (!targetSSA.empty()) {
                        relevantSSANames.insert(targetSSA);
                    }
                }
                
                // Get RHS variables  
                std::vector<std::string> rhsVars;
                extractVariablesFromExpr(binOp->getRHS(), rhsVars);
                for (const auto& rhsBaseName : rhsVars) {
                    std::string rhsSSA = findLatestSSAName(rhsBaseName);
                    if (!rhsSSA.empty()) {
                        relevantSSANames.insert(rhsSSA);
                    }
                }
            }
        }
        
        // STEP 2: Add variables from path conditions
        std::vector<std::string> pathSSANames = extractSSANamesFromPathConditions();
        for (const auto& ssaName : pathSSANames) {
            if (!ssaName.empty()) {
                relevantSSANames.insert(ssaName);
            }
        }
        
        // STEP 3: Fixed-point iteration - for each variable in set, add its used variables
        bool changed = true;
        int iteration = 0;
        const int MAX_ITERATIONS = 10;
        
        while (changed && iteration < MAX_ITERATIONS) {
            changed = false;
            iteration++;
            
            // Create snapshot of current set
            std::vector<std::string> current(relevantSSANames.begin(), relevantSSANames.end());
            
            for (const auto& ssaName : current) {
                // Look up definition in varDefs
                // METHOD A: Use stored usedVars (if they're correct)
                auto it = varDefs.find(ssaName);
                if (it != varDefs.end()) {
                    const DefinitionInfo& def = it->second;
                    for (const auto& var : def.usedVars) {
                        if (relevantSSANames.insert(var).second) {
                                changed = true;
                         }
                    }
                    
                }
            }
        }
        
        return relevantSSANames;
    }

    std::vector<std::string> extractSSANamesFromPathConditions() const {
        std::vector<std::string> ssaNames;
        
        std::stack<std::string> tempStack = conditionStack;
        while (!tempStack.empty()) {
            std::string cond = tempStack.top();
            
            if (!cond.empty()) {
                size_t start = 0;
                while (start < cond.length()) {
                    if (std::isalpha(cond[start]) || cond[start] == '_') {
                        size_t end = start;
                        while (end < cond.length() && 
                            (std::isalnum(cond[end]) || cond[end] == '_')) {
                            end++;
                        }
                        
                        if (end > start) {
                            std::string token = cond.substr(start, end - start);
                            
                            // Check if it's an SSA name (has _digits suffix)
                            size_t lastUnderscore = token.find_last_of('_');
                            if (lastUnderscore != std::string::npos && 
                                lastUnderscore > 0 && 
                                lastUnderscore < token.length() - 1) {
                                
                                std::string suffix = token.substr(lastUnderscore + 1);
                                bool isAllDigits = true;
                                for (char c : suffix) {
                                    if (!std::isdigit(c)) {
                                        isAllDigits = false;
                                        break;
                                    }
                                }
                                
                                if (isAllDigits) {
                                    ssaNames.push_back(token);
                                }
                            }
                        }
                        start = end;
                    } else {
                        start++;
                    }
                }
            }
            tempStack.pop();
        }
        
        // Remove duplicates
        if (!ssaNames.empty()) {
            std::sort(ssaNames.begin(), ssaNames.end());
            auto last = std::unique(ssaNames.begin(), ssaNames.end());
            ssaNames.erase(last, ssaNames.end());
        }
        
        return ssaNames;
    }

    
    std::string getBaseNameFromSSA(const std::string& ssaName) const {
        if (ssaName.empty()) return ssaName;
        
        size_t lastUnderscore = ssaName.find_last_of('_');
        if (lastUnderscore == std::string::npos || lastUnderscore == 0 || 
            lastUnderscore >= ssaName.length() - 1) {
            return ssaName;
        }
        
        std::string suffix = ssaName.substr(lastUnderscore + 1);
        bool isAllDigits = true;
        for (char c : suffix) {
            if (!std::isdigit(c)) {
                isAllDigits = false;
                break;
            }
        }
        
        if (isAllDigits) {
            return ssaName.substr(0, lastUnderscore);
        }
        return ssaName;
    }
    std::string findLatestSSAName(const std::string& baseName) const {
        int maxVersion = -1;
        std::string latestSSA = "";
        
        for (const auto& [ssaName, def] : varDefs) {
            if (def.ssaVar.name == baseName) {
                if (def.ssaVar.version > maxVersion) {
                    maxVersion = def.ssaVar.version;
                    latestSSA = ssaName;
                }
            }
        }
        
        return latestSSA;
    }
   

    SMTTargetContext buildSMTContext() const {
    SMTTargetContext context;
    
    // Set target path conditions
    std::stack<std::string> tempStack = conditionStack;
    while (!tempStack.empty()) {
        context.targetPathConditions.push_back(tempStack.top());
        tempStack.pop();
    }
    std::reverse(context.targetPathConditions.begin(), context.targetPathConditions.end());
    
    // DEBUG: Print target statement info
    llvm::outs() << "DEBUG: Target statement type: " << globalTargetStmt->getStmtClassName() << "\n";
    
    // Find the actual assignment statement if we have a literal
    const clang::Stmt* actualTarget = globalTargetStmt;
    
    // If target is a literal, try to find its parent assignment
    if (isa<clang::IntegerLiteral>(globalTargetStmt) || 
        isa<clang::FloatingLiteral>(globalTargetStmt) ||
        isa<clang::StringLiteral>(globalTargetStmt)) {
        
        llvm::outs() << "DEBUG: Target is a literal, searching for parent assignment...\n";
        // You'll need to implement parent lookup or store parent info
        // For now, we'll assume the target should be the assignment
        // This is a limitation of the current approach
    }
    
    context.targetStmt = actualTarget;
    
    // Handle different target statement types
    if (const auto* callExpr = dyn_cast<clang::CallExpr>(actualTarget)) {
        // Handle function calls
        for (unsigned i = 0; i < callExpr->getNumArgs(); ++i) {
            std::vector<std::string> argVars;
            extractVariablesFromExpr(callExpr->getArg(i), argVars);
            context.targetVariables.insert(context.targetVariables.end(), argVars.begin(), argVars.end());
        }
    }
    else if (const auto* binOp = dyn_cast<clang::BinaryOperator>(actualTarget)) {
        // Handle assignments and other binary operations
        llvm::outs() << "DEBUG: Found BinaryOperator\n";
        if (binOp->isAssignmentOp()) {
            // For assignments, the target variables are on the LHS
            extractVariablesFromExpr(binOp->getLHS(), context.targetVariables);
            // Also include RHS variables as they influence the assignment
            extractVariablesFromExpr(binOp->getRHS(), context.targetVariables);
        } else {
            // For other binary ops, get variables from both sides
            extractVariablesFromExpr(binOp->getLHS(), context.targetVariables);
            extractVariablesFromExpr(binOp->getRHS(), context.targetVariables);
        }
    }
    else if (const auto* dre = dyn_cast<clang::DeclRefExpr>(actualTarget)) {
        // Handle variable references
        llvm::outs() << "DEBUG: Found DeclRefExpr: " << dre->getNameInfo().getAsString() << "\n";
        context.targetVariables.push_back(dre->getNameInfo().getAsString());
    }
    else {
        llvm::outs() << "DEBUG: Target is unsupported statement type!\n";
        // Try to extract variables anyway
        extractVariablesFromExpr(dyn_cast_or_null<clang::Expr>(actualTarget), context.targetVariables);
    }
    
    // Build variable definitions
    for (const auto& [ssaName, def] : varDefs) {
        SMTVariableDefinition smtDef;
        smtDef.baseName = def.ssaVar.name;
        smtDef.ssaName = def.ssaVar.ssaName();
        smtDef.version = def.ssaVar.version;
        smtDef.definingExpr = def.definingExpr;
        smtDef.defStmt = def.defStmt;
        smtDef.exprString = def.exprString;
        smtDef.pathConditions = def.conditionContext;
        smtDef.line = def.line;
        smtDef.stmtID = def.stmtID;
        
        context.variableDefinitions[smtDef.baseName].push_back(smtDef);
    }
    
    // Compute relevant variables
    
    
    llvm::outs() << "DEBUG: About to compute relevant variables\n";
    context.relevantVariables =  computeRelevantVariablesWithSSANames();//computeRelevantVariablesUsingSymtab();
    llvm::outs() << "DEBUG: Finished computing relevant variables\n";
    
    return context;
}

    void printSMTContext(const SMTTargetContext& context) const {
    llvm::outs() << "\n=== SMT Context ===\n";
    llvm::outs() << "Target path conditions:\n";
    for (const auto& cond : context.targetPathConditions) {
        llvm::outs() << "  " << cond << "\n";
    }
    
    llvm::outs() << "Target variables: ";
    for (const auto& var : context.targetVariables) {
        llvm::outs() << var << " ";
    }
    llvm::outs() << "\n";
    
    llvm::outs() << "Relevant variables: ";
    for (const auto& var : context.relevantVariables) {
        llvm::outs() << var << " ";
    }
    llvm::outs() << "\n";
}

    std::vector<std::string> extractBaseNamesFromPathConditions() const {
        std::vector<std::string> baseNames;
        
        // Work with a copy of the condition stack
        std::stack<std::string> tempStack = conditionStack;
        
        while (!tempStack.empty()) {
            std::string cond = tempStack.top();
            
            // Skip empty conditions
            if (!cond.empty()) {
                size_t pos = 0;
                
                // Simple approach: look for patterns like "varname_digits"
                while (pos < cond.length()) {
                    // Find start of potential variable name
                    if (std::isalpha(cond[pos]) || cond[pos] == '_') {
                        size_t start = pos;
                        
                        // Extract variable-like token
                        while (pos < cond.length() && 
                            (std::isalnum(cond[pos]) || cond[pos] == '_')) {
                            pos++;
                        }
                        
                        std::string token = cond.substr(start, pos - start);
                        
                        // Check if it has SSA suffix (_ followed by digits)
                        size_t underscorePos = token.find_last_of('_');
                        if (underscorePos != std::string::npos && 
                            underscorePos > 0 && 
                            underscorePos < token.length() - 1) {
                            
                            std::string suffix = token.substr(underscorePos + 1);
                            bool isAllDigits = true;
                            for (char c : suffix) {
                                if (!std::isdigit(c)) {
                                    isAllDigits = false;
                                    break;
                                }
                            }
                            
                            if (isAllDigits) {
                                std::string baseName = token.substr(0, underscorePos);
                                if (!baseName.empty()) {
                                    baseNames.push_back(baseName);
                                }
                            }
                        }
                    } else {
                        pos++;
                    }
                }
            }
            
            tempStack.pop();
        }
        
        // Remove duplicates
        if (!baseNames.empty()) {
            std::sort(baseNames.begin(), baseNames.end());
            auto last = std::unique(baseNames.begin(), baseNames.end());
            baseNames.erase(last, baseNames.end());
        }
        
        return baseNames;
    }


    SSAVarInfo getSSAVarInfoFromVarDefs(const std::string& ssaName) const {
        SSAVarInfo info;
        auto it = varDefs.find(ssaName);
        if (it != varDefs.end()) {
            const DefinitionInfo& def = it->second;
            info.found = true;
            info.baseName = def.ssaVar.name;
            info.version = def.ssaVar.version;
            info.exprString = def.exprString;
            info.definingExpr = def.definingExpr;
            info.pathConditions = def.conditionContext;
        }
        return info;
    }

};
