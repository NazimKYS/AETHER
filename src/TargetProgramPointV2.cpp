#pragma once
#include "main.h"



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
    
    // Add default constructor (required for unordered_map)
    SSAVariable() : version(0) {}
    
    // Constructor for existing code
    SSAVariable(const std::string& n, int v) : name(n), version(v) {}
    
    // Constructor with definition info
    SSAVariable(const std::string& n, int v, const std::string& expr, 
                const clang::Expr* defExpr = nullptr) 
        : name(n), version(v), exprString(expr), definingExpr(defExpr) {}
};



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
    std::unordered_map<std::string, std::vector<SMTVariableDefinition>> variableDefinitions;
    std::vector<std::string> targetVariables;
    std::set<std::string> relevantVariables;
};

struct DefinitionInfo {
    SSAVariable ssaVar;
    unsigned line;
    unsigned stmtID;
    std::string exprString;
    std::vector<SSAVariable> usedVars;
    std::vector<std::string> conditionContext;
    
    // NEW: Store original AST nodes for SMT generation
    const clang::Expr* definingExpr = nullptr;    // RHS of assignment
    const clang::Expr* conditionExpr = nullptr;   // For if conditions
    const clang::Stmt* defStmt = nullptr;        // The statement itself
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
    std::unordered_map<std::string, int> save() const {
        return currentVersion; // copy current state
    }
    std::unordered_map<std::string, SSAVariable> getCurrentState() const {
      std::unordered_map<std::string, SSAVariable> state;
      for (const auto& [name, history] : versionHistory) {
          if (!history.empty()) {
              state[name] = history.back();
          }
      }
      return state;
    }

    // 🔑 NEW: Restore version state
    void restore(const std::unordered_map<std::string, SSAVariable>& state) {
      currentVersion.clear();
      versionHistory.clear();
      
      for (const auto& [name, var] : state) {
          currentVersion[name] = var.version;
          versionHistory[name] = {var}; // simplified - you might want full history
      }
    }

    const std::unordered_map<std::string, std::vector<SSAVariable>>& getAll() const {
        return versionHistory;
    }
    // In SSASymbolTable class


// Add this method to SSASymbolTable
    std::vector<SSAVariable> getAllDefinitions(const std::string& baseName) const {
        auto it = versionHistory.find(baseName);
        if (it != versionHistory.end()) {
            return it->second; // assuming versionHistory stores VariableDefinition
        }
        return {};
    }

// Get latest definition
    const SSAVariable* getLatestDefinition(const std::string& baseName) const {
    auto defs = getAllDefinitions(baseName);
    if (!defs.empty()) {
        return &defs.back();
    }
    return nullptr;
}

// Add this method to your SSASymbolTable class
    SSAVariable defineWithDefinition(const std::string& varName, const std::string& exprString,  const clang::Expr* definingExpr = nullptr,  const std::vector<std::string>& pathConditions = {}) {

    SSAVariable newVar = define(varName);
    // Update the last definition with extra info
    if (!versionHistory[varName].empty()) {
        auto& lastDef = versionHistory[varName].back();
        lastDef.exprString = exprString;
        lastDef.definingExpr = definingExpr;
        lastDef.pathConditions = pathConditions;
    }
    return newVar;
}


    void printSymbolTable() const {
    llvm::outs() << "\n=== Symbol Table Content ===\n";
    if (versionHistory.empty()) {
        llvm::outs() << "(empty)\n";
        return;
    }
    
    for (const auto& [varName, definitions] : versionHistory) {
        llvm::outs() << "Variable: " << varName << "\n";
        for (size_t i = 0; i < definitions.size(); ++i) {
            const auto& def = definitions[i];
            llvm::outs() << "  Version " << def.version << " (" << def.ssaName() << "):\n";
            llvm::outs() << "    Expression: " << def.exprString << "\n";
            if (!def.pathConditions.empty()) {
                llvm::outs() << "    Path conditions:\n";
                for (const auto& cond : def.pathConditions) {
                    llvm::outs() << "      " << cond << "\n";
                }
            } else {
                llvm::outs() << "    Path conditions: (none)\n";
            }
            llvm::outs() << "    Has definingExpr: " << (def.definingExpr ? "yes" : "no") << "\n";
        }
    }
    llvm::outs() << "===========================\n\n";
}

    const SSAVariable* getDefinitionBySSAName(const std::string& ssaName) const {
        // Handle empty string
        if (ssaName.empty()) {
            return nullptr;
        }
        
        // Find the last underscore by manual search
        size_t lastUnderscore = std::string::npos;
        for (size_t i = ssaName.length(); i > 0; --i) {
            if (ssaName[i - 1] == '_') {
                lastUnderscore = i - 1;
                break;
            }
        }
        
        // Must have at least one underscore and it can't be at the beginning
        if (lastUnderscore == std::string::npos || lastUnderscore == 0) {
            return nullptr;
        }
        
        // Extract the suffix after the last underscore
        if (lastUnderscore + 1 >= ssaName.length()) {
            return nullptr;
        }
        
        std::string suffix = ssaName.substr(lastUnderscore + 1);
        
        // Suffix must be non-empty and contain only digits
        if (suffix.empty()) {
            return nullptr;
        }
        
        bool isAllDigits = true;
        for (char c : suffix) {
            if (!std::isdigit(c)) {
                isAllDigits = false;
                break;
            }
        }
        
        if (!isAllDigits) {
            return nullptr;
        }
        
        // Extract base name (everything before the last underscore)
        std::string baseName = ssaName.substr(0, lastUnderscore);
        if (baseName.empty()) {
            return nullptr;
        }
        
        // Convert version string to integer manually (no exceptions)
        int version = 0;
        int multiplier = 1;
        
        // Convert from right to left
        for (int i = static_cast<int>(suffix.length()) - 1; i >= 0; --i) {
            version += (suffix[i] - '0') * multiplier;
            multiplier *= 10;
        }
        
        // Look up all definitions for this base name
        const auto& defs = getAllDefinitions(baseName);
        
        // Find the definition with matching version
        for (const auto& def : defs) {
            if (def.version == version) {
                return &def;
            }
        }
        
        // Not found
        return nullptr;
}
/*void clear() {
    versionHistory.clear();
    currentVersion.clear();
}
*/

// Get complete definition info by SSA name
    SSAVarInfo getSSAVarInfo(const std::string& ssaName) const {
    SSAVarInfo info;
    
    const SSAVariable* def = getDefinitionBySSAName(ssaName);
    if (!def) {
        return info;
    }
    
    info.found = true;
    info.baseName = def->name;
    info.version = def->version;
    info.exprString = def->exprString;
    info.definingExpr = def->definingExpr;
    info.pathConditions = def->pathConditions;
    
    return info;
}
};




// Helper functions (implement these as shown in previous answer)

class TargetProgramPoint {
    
private:
    SSASymbolTable symtab;
    std::unordered_map<unsigned, std::vector<DefinitionInfo>> lineToDefinitions;
    std::unordered_map<std::string, DefinitionInfo> varDefs;
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
        const clang::Expr* conditionExpr;
        bool isTrueBranch; // true = then, false = else
        std::string smtString; // optional: precomputed string
  };

  std::vector<PathCondition> pathConditions; // ← NEW

    TargetProgramPoint(){};
    TargetProgramPoint(const Stmt *targetStmt, ASTContext &astContext) {
        globalTargetStmt = targetStmt;
        globalAstContext = &astContext;
        sharedASTContext = &astContext;
        //cout << "from target " << globalTargetStmt->getID(*globalAstContext)<< "\n";
        // globalTargetStmt->dump();
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
    pathConditions.clear();
    for (unsigned int i = 0; i < vectorOfParentStmt.size(); ++i) {
      //cout<<" for loop getConditionPath index "<<i<<" \n";
      const Stmt *st = vectorOfParentStmt[i].st;
      if (isa<IfStmt>(st)) {
        const IfStmt *ifStmt = cast<IfStmt>(st);
        condition = ifStmt->getCond();
        if (!condition) continue;

        bool isTrueBranch = false;
        //checking condition state disable comment after mapping new nodes of dataStructure
        
        if ( vectorOfParentStmt.size()> (i - 1) ) {
          StmtAttributes nextStmt = vectorOfParentStmt[i - 1];
          if (nextStmt.id == (ifStmt->getThen())->getID(*globalAstContext)) {
            // comment line 260 & 266
            cout<<"vectorSize = "<<vectorOfParentStmt.size()<<" access element vector[ "<<i - 1 <<" ]\n";
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
       
        
        PathCondition pc;
        pc.conditionExpr = condition;
        pc.isTrueBranch = isTrueBranch;

        const Stmt *conditionStmt= ifStmt->getCond(); 
        if(condState=="Not"){
          
          NodeTool *tree = new NodeTool(conditionStmt);
          NodeTool root(condState,tree);
          //cout<<"roooooooooooooot Not \n"<<root.pyz3ApiFlatten()<<"\n";
          pc.smtString = root.pyz3ApiFlatten();
        }else{
          //NodeTool *tree = new NodeTool(conditionStmt); /* to avoid ==10661==ERROR: LeakSanitizer: detected memory leaks Direct leak of 96 byte(s) in 1 object(s) allocated from:    1 0x5d81cfdda85a in TargetProgramPoint::getConditionPathV2() src/TargetProgramPoint.cpp:315 */
          NodeTool tree(conditionStmt);
          //cout<<"roooooooooooooot true \n"<<tree.pyz3ApiFlatten()<<"\n";
          pc.smtString = tree.pyz3ApiFlatten();
          
        }
        pathConditions.push_back(pc);
        binaryOpToStrV2(conditionStmt,globalAstContext);
        
        // testing binaryStrV2 
        /*
        allExpressions.push_back(
            condState + "( " +  binaryOpToStr(conditionStmt, srcMgr, globalAstContext)  + " )");
        //cout<<"allexpression size"<< allExpressions.size()<<"\n";*/
      }  
    }

    // disable printing final expression while testing new datStructure
    /*if (allExpressions.size() == 0) {
      cout << "the target instruction will be executed at each call of "<<
      parentFunctionOfProgramPoint->getNameInfo().getName().getAsString()
              <<" function \n";
    } else {

      for (unsigned int i = 0; i < allExpressions.size() - 1; i++) {
        expression = expression + allExpressions[i] + " and ";
      }
      cout << expression + allExpressions[allExpressions.size() - 1] << "\n";
    }*/

  }
  

    bool VisitBinaryOperator(BinaryOperator* binOp, const Stmt* originalStmt = nullptr) {
    if (binOp->isAssignmentOp()) {
        Expr* lhs = binOp->getLHS()->IgnoreParenImpCasts();
        Expr* rhs = binOp->getRHS()->IgnoreParenImpCasts();

        if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(lhs)) {
            std::string varName = declRef->getNameInfo().getAsString();

            // Collect used variables (you'll need to pass current path state)
            std::vector<SSAVariable> usedVars;
            collectUsedVars(rhs, usedVars); // Update this later if needed

            // Create substituted RHS string
            std::string rawRHS;
            llvm::raw_string_ostream ss(rawRHS);
            rhs->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy());
            ss.flush();

            std::string substituted = ss.str();
            for (const auto& used : usedVars) {
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
            SSAVariable newDef = symtab.defineWithDefinition(
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

    void printCurrentSymbolTable() const {
        llvm::outs() << "\n=== Current Symbol Table at Target ===\n";
        symtab.printSymbolTable();
    }

    void exploreFunctionByStmt() {
        const FunctionDecl* func = parentFunctionOfProgramPoint;
        if (!func || !func->hasBody()) {
            return;
        }
        
        const Stmt* body = func->getBody();
        
        // Reset state before traversal
        targetFound = false;
        
        // Perform traversal
        bool found = visitAllStmts(body);
        
        if (found) {
            llvm::outs() << "\n=== Building SMT Context ===\n";
            SMTTargetContext smtContext = buildSMTContext();
            printSMTContext(smtContext);
            
            // ✅ USE THE NEW FUNCTION INSTEAD
            SSAVarInfo info = getSSAVarInfoFromVarDefs("serviceId_0");
            if (info.found) {
                llvm::outs() << "Base name: " << info.baseName << "\n";
                llvm::outs() << "Version: " << info.version << "\n";
                llvm::outs() << "Expression: " << info.exprString << "\n";
            }
            
            // Print variable history (this works fine)
            printVariableHistory();
        } else {
            llvm::errs() << "Target statement not found!\n";
        }  
        

    }


  bool visitAllStmts(const Stmt* stmt) {
    if (!stmt) return false;

    if (stmt == globalTargetStmt) {
        llvm::outs() << "******* TARGET STMT FOUND — STOPPING \n";
        printConditionStackTopToBottom(conditionStack);
        targetFound = true;
        return true;
    }

    if (isa<IfStmt>(stmt)) {
        IfStmt* ifStmt = const_cast<IfStmt*>(cast<IfStmt>(stmt));
        Expr* condExpr = ifStmt->getCond();
        
        std::vector<SSAVariable> usedVars;
        collectUsedVars(condExpr, usedVars);

        std::string condText;
        llvm::raw_string_ostream ss(condText);
        condExpr->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy());
        std::string rewrittenCond = rewriteWithSSA(ss.str(), usedVars);

        SSAVariable condVar = symtab.define("cond");
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

        if (const Stmt* thenBody = ifStmt->getThen()) {
            conditionStack.push(rewrittenCond);
            if (visitAllStmts(thenBody)) return true;
            conditionStack.pop();
        }

        if (const Stmt* elseBody = ifStmt->getElse()) {
            conditionStack.push("!(" + rewrittenCond + ")");
            if (visitAllStmts(elseBody)) return true;
            conditionStack.pop();
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


  void collectVariablesFromTarget(std::set<std::string>& vars) {
    if (const auto* callExpr = dyn_cast<clang::CallExpr>(globalTargetStmt)) {
        for (unsigned i = 0; i < callExpr->getNumArgs(); ++i) {
            std::vector<std::string> tempVars;
            extractVariablesFromExpr(callExpr->getArg(i), tempVars);
            // Add all variables from tempVars to the set
            vars.insert(tempVars.begin(), tempVars.end());
        }
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
        
        // Handle DeclRefExpr (direct variable reference)
        if (const auto* dre = dyn_cast<clang::DeclRefExpr>(expr)) {
            std::string name = dre->getNameInfo().getAsString();
            if (!name.empty()) {
                // Check if this is an SSA name (has _digits suffix)
                size_t lastUnderscore = name.find_last_of('_');
                if (lastUnderscore != std::string::npos && 
                    lastUnderscore > 0 && 
                    lastUnderscore < name.length() - 1) {
                    
                    std::string suffix = name.substr(lastUnderscore + 1);
                    bool isAllDigits = true;
                    for (char c : suffix) {
                        if (!std::isdigit(c)) {
                            isAllDigits = false;
                            break;
                        }
                    }
                    
                    if (isAllDigits) {
                        // Extract base name
                        std::string baseName = name.substr(0, lastUnderscore);
                        if (!baseName.empty()) {
                            vars.push_back(baseName);
                        }
                    } else {
                        vars.push_back(name);
                    }
                } else {
                    vars.push_back(name);
                }
            }
            return;
        }
        
        // Handle ImplicitCastExpr (very common wrapper in Clang AST)
        if (const auto* ice = dyn_cast<clang::ImplicitCastExpr>(expr)) {
            extractVariablesFromExpr(ice->getSubExpr(), vars);
            return;
        }
        
        // Handle CStyleCastExpr
        if (const auto* cce = dyn_cast<clang::CStyleCastExpr>(expr)) {
            extractVariablesFromExpr(cce->getSubExpr(), vars);
            return;
        }
        
        // Handle ParenExpr (parentheses)
        if (const auto* pe = dyn_cast<clang::ParenExpr>(expr)) {
            extractVariablesFromExpr(pe->getSubExpr(), vars);
            return;
        }
        
        // Recurse into children for complex expressions
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


    std::set<std::string> computeRelevantVariablesUsingSymtab() const {
    std::set<std::string> relevant;
    
    // Add target variables (base names)
    if (const auto* binOp = dyn_cast<clang::BinaryOperator>(globalTargetStmt)) {
        if (binOp->isAssignmentOp()) {
            if (const auto* lhs = dyn_cast<clang::DeclRefExpr>(binOp->getLHS())) {
                std::string name = lhs->getNameInfo().getAsString();
                if (!name.empty()) {
                    relevant.insert(name);
                }
            }
        }
    }
    
    // Add variables from path conditions
    std::vector<std::string> pathVars = extractBaseNamesFromPathConditions();
    for (const auto& var : pathVars) {
        if (!var.empty()) {
            relevant.insert(var);
        }
    }
    
    // Transitive closure using symbol table
    bool changed = true;
    int iteration = 0;
    const int MAX_ITERATIONS = 10; // ⚠️ Reduce this to prevent memory explosion
    
    while (changed && iteration < MAX_ITERATIONS) {
        changed = false;
        iteration++;
        
        // Create a snapshot of current relevant variables
        std::vector<std::string> current(relevant.begin(), relevant.end());
        
        for (const auto& var : current) {
            if (var.empty()) continue;
            
            const SSAVariable* def = symtab.getLatestDefinition(var);
            if (def && def->definingExpr) {
                std::vector<std::string> rhsVars;
                extractVariablesFromExpr(def->definingExpr, rhsVars);
                
                for (const auto& rhsVar : rhsVars) {
                    if (!rhsVar.empty()) {
                        // Only add if it's not already in the set
                        if (relevant.find(rhsVar) == relevant.end()) {
                            relevant.insert(rhsVar);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    
    if (iteration >= MAX_ITERATIONS) {
        llvm::outs() << "WARNING: Reached maximum iterations in backward analysis\n";
    }
    
    return relevant;
}
    /*llvm::outs() << "DEBUG: Final relevant variables: ";
    for (const auto& var : relevant) {
        if (!var.empty()) {
            llvm::outs() << "'" << var << "' ";
        }
    }
    llvm::outs() << "\n";
    std::set<std::string> validated;
    for (const auto& var : relevant) {
        if (!var.empty() && var.length() < 1000) { // reasonable length check
            validated.insert(var);
        }
    }
    return validated;
    

}*/

    std::string getBaseNameFromSSA(const std::string& ssaName) const {
    size_t pos = ssaName.find('_');
    if (pos != std::string::npos && pos > 0) {
        std::string suffix = ssaName.substr(pos + 1);
        if (!suffix.empty() && std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
            return ssaName.substr(0, pos);
        }
    }
    return ssaName; // not an SSA name
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
    context.relevantVariables = computeRelevantVariablesUsingSymtab();
    
    llvm::outs() << "DEBUG: About to compute relevant variables\n";
    context.relevantVariables = computeRelevantVariablesUsingSymtab();
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
    /*
public:
// Implementation
    ~TargetProgramPoint() {
        // Clear condition stack safely
        while (!conditionStack.empty()) {
            conditionStack.pop();
        }
        // Clear symbol table
        symtab.clear(); // You'll need to add this method
        
        // Clear other containers
        varDefs.clear();
        lineToDefinitions.clear();
    }*/
};
