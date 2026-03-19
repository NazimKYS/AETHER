#pragma once
#include "main.h"
#include "llvm/ADT/SmallString.h"

int getCurrentSsaVersionOfVariable(string baseName);




class SmtScript {
public:
    // Default constructor
    SmtScript() = default;
    
    // Data members
    std::vector<std::string> variableDeclarations;
    std::vector<std::string> variableDefinitions;
    
    // Methods
    void pushIfNotInDeclarationVector(const std::string& decl);
    void pushIfNotInDefinitionVector(const std::string& def);
    void printVariableDeclarations();
    void printVariableDefinitions();
    std::string generateFreshVar(const std::string& baseName = "fresh");
    void ensureVariableDeclared(const std::string& varName, const std::string& type = "Int");
    
private:
    std::unordered_map<std::string, int> freshVarCounters;
};

struct PathCondition;
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
    string varType;
    std::string exprString;
    std::vector<std::string> usedVars;
    std::vector<std::string> conditionContext;
    // bool hasExplicitCast
    bool isCondtionalDefinition;
    std::vector<PathCondition> globalConditionContext;

    const clang::Stmt* defStmt = nullptr;
    
    string smtDefinitionExpression;

    static int freshVarKcounter;
    static int freshVarXbarcounter;
    static std::vector<std::string> freshVarNames;


    DefinitionInfo() = default;

    // 2. CORE CONSTRUCTOR (enforce critical fields)
    //    Use when you have SSA variable + location + type upfront
    DefinitionInfo(const SSAVariable& var, 
                   unsigned ln, 
                   unsigned id, 
                   const std::string& type)
        : ssaVar(var), line(ln), stmtID(id), varType(type) 
    {}

    DefinitionInfo(const SSAVariable& ssaVar,        // ← const ref (no copy)
                   unsigned line, 
                   unsigned stmtID,
                   const std::string& varType,       // ← const ref
                   const std::string& exprString,    // ← const ref
                   const std::vector<std::string>& usedVars,      // ← const ref
                   const std::vector<std::string>& conditionContext)  // ← const ref
        : ssaVar(ssaVar),          // Parameter name = member name (allowed in initializer list)
          line(line),              // Safe: parameter shadows member, but initializer resolves correctly
          stmtID(stmtID),
          varType(varType),
          exprString(exprString),
          usedVars(usedVars),
          conditionContext(conditionContext)
          // Pointers auto-initialized to nullptr via in-class initializers
    {}

    DefinitionInfo(const SSAVariable& ssaVar,
                   unsigned line,
                   unsigned stmtID,
                   const std::string& varType,
                   const std::string& exprString,
                   const std::vector<std::string>& usedVars,
                   const std::vector<std::string>& conditionContext,
                   const clang::Stmt* defStmt)
        : ssaVar(ssaVar), line(line), stmtID(stmtID), varType(varType),
          exprString(exprString), usedVars(usedVars), 
          conditionContext(conditionContext), 
          defStmt(defStmt)
    {
        if (!defStmt) {
            return;
        }

        // ONLY proceed if BinaryOperator (or other explicitly supported types)
        if (!llvm::isa<clang::BinaryOperator>(defStmt)) {
            return;
        }

        const BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(defStmt);
            //Expr *lhs = (binOpStmt->getLHS())->IgnoreImpCasts();
        const Expr *rhs = (binOpStmt->getRHS())->IgnoreImpCasts();
        
        smtDefinitionExpression = handleArithmeticWithOverflow(rhs);

       
    }
 

    std::string getPyZ3Declaration() const {
        string def ="#NULL not handled data type "+ varType;
        string ssaName = ssaVar.ssaName();
        if(varType=="int" || varType=="long" || varType=="short" || varType=="long long" ){

            def = ssaName + " = Int('"+ssaName+"')" ; //serviceId = Int('serviceId') 
        }
        return def;
    }

    std::string getPyZ3Definition(const clang::Stmt* defStmt) const {
        
        string def ="#NULL not handled data type or Conditional Def"+ varType;
        string ssaName = ssaVar.ssaName();
        def = smtDefinitionExpression   ;
        return def;
    }

    void copyCondPathFromstackToVector(std::stack<PathCondition> &gloablPathConditions){
        
        globalConditionContext.clear();
        
        if (gloablPathConditions.empty()){
            
            isCondtionalDefinition=false;
            return;  // Early exit
        }else{
            isCondtionalDefinition=true;
            std::stack<PathCondition> temp = gloablPathConditions;
            std::vector<PathCondition> reversed;
            reversed.reserve(temp.size());
            
            while (!temp.empty()) {
                reversed.push_back(std::move(temp.top()));
                temp.pop();
            }
            globalConditionContext.assign(reversed.rbegin(), reversed.rend());
        }
    }
   
    static std::string handleArithmeticWithOverflow(const clang::Expr* expr) {
    if (const auto* binOp = dyn_cast<clang::BinaryOperator>(expr)) {
        if (binOp->isAdditiveOp() || binOp->isMultiplicativeOp()) {
            std::string lhs = exprToPyZ3(binOp->getLHS());
            std::string rhs = exprToPyZ3(binOp->getRHS());
            // If either operand is unresolvable (e.g. a function call), treat the
            // whole expression as symbolic — no definition, no fresh overflow vars.
            if (lhs.empty() || rhs.empty()) return "";
            std::string op = getPyZ3BinaryOperator(binOp->getOpcode());
            std::string originalExpr = "(" + lhs + " " + op + " " + rhs + ")";
            
            // Generate fresh variables
            std::string kVar = "k" + std::to_string(++freshVarKcounter);
            std::string xbarVar = "xbar" + std::to_string(++freshVarXbarcounter);
            freshVarNames.push_back(kVar);
            freshVarNames.push_back(xbarVar);
            
            // Get bit width
            unsigned bitWidth = getBitWidthForType(expr->getType(), sharedExecutionEnv);
            std::string modulus = std::to_string(bitWidth);
            
            // Build overflow constraint
            std::string powerOfTwo = "2**" + modulus;
            std::string overflowConstraint = originalExpr + " == " + powerOfTwo + " * " + kVar + " + " + xbarVar;
            std::string condition = "And(" + overflowConstraint + ", " + kVar + " > 0)";
            
            return "If(" + condition + ", " + xbarVar + ", " + originalExpr + ")";
        }
    }
    return exprToPyZ3(expr);
}
    static std::string exprToPyZ3(const clang::Expr* expr) {
        if (!expr) return "";

        // Handle implicit casts (very common in Clang AST)
        if (const auto* ice = dyn_cast<clang::ImplicitCastExpr>(expr)) {
            return exprToPyZ3(ice->getSubExpr());
        }

        // Handle explicit casts
        if (const auto* cce = dyn_cast<clang::CStyleCastExpr>(expr)) {
            return exprToPyZ3(cce->getSubExpr());
        }

        // Handle parentheses
        if (const auto* pe = dyn_cast<clang::ParenExpr>(expr)) {
            std::string inner = exprToPyZ3(pe->getSubExpr());
            if (inner.empty()) return "";
            return "(" + inner + ")";
        }

        // Handle binary operators (arithmetic, comparison, logical)
        if (const auto* binOp = dyn_cast<clang::BinaryOperator>(expr)) {
            std::string lhs = exprToPyZ3(binOp->getLHS());
            std::string rhs = exprToPyZ3(binOp->getRHS());
            // Propagate unresolvable operands upward
            if (lhs.empty() || rhs.empty()) return "";
            std::string op = getPyZ3BinaryOperator(binOp->getOpcode());

            if (binOp->isComparisonOp()) {
                return lhs + " " + op + " " + rhs;
            } else if (binOp->isAdditiveOp() || binOp->isMultiplicativeOp()) {
                return "(" + lhs + " " + op + " " + rhs + ")";
            } else if (binOp->getOpcode() == clang::BO_LAnd) {
                return "And(" + lhs + ", " + rhs + ")";
            } else if (binOp->getOpcode() == clang::BO_LOr) {
                return "Or(" + lhs + ", " + rhs + ")";
            } else if (binOp->getOpcode() == clang::BO_EQ) {
                return lhs + " == " + rhs;
            } else if (binOp->getOpcode() == clang::BO_NE) {
                return lhs + " != " + rhs;
            }
            return "(" + lhs + " " + op + " " + rhs + ")";
        }

        // Handle unary operators
        if (const auto* unaryOp = dyn_cast<clang::UnaryOperator>(expr)) {
            std::string sub = exprToPyZ3(unaryOp->getSubExpr());
            if (sub.empty()) return "";
            if (unaryOp->getOpcode() == clang::UO_Minus) return "(-" + sub + ")";
            if (unaryOp->getOpcode() == clang::UO_LNot)  return "Not(" + sub + ")";
            return sub;
        }

        // Handle variable references (DeclRefExpr)
        if (const auto* dre = dyn_cast<clang::DeclRefExpr>(expr)) {
            std::string varName = dre->getNameInfo().getAsString();
            int ver = getCurrentSsaVersionOfVariable(varName);
            if (ver < 0) return varName; // unknown variable — use base name as-is
            return varName + "_" + std::to_string(ver);
        }

        // Handle integer literals
        if (const auto* intLit = dyn_cast<clang::IntegerLiteral>(expr)) {
            llvm::SmallString<32> str;
            intLit->getValue().toString(str, 10, false);
            return std::string(str.c_str());
        }

        // Handle floating point literals
        if (const auto* floatLit = dyn_cast<clang::FloatingLiteral>(expr)) {
            std::string floatStr;
            llvm::raw_string_ostream ss(floatStr);
            floatLit->getValue().print(ss);
            return ss.str();
        }

        // CallExpr and any other unresolvable expression → treat as symbolic (no definition)
        return "";
    }

    // Helper to map Clang binary operators to PyZ3 operators
    static std::string getPyZ3BinaryOperator(clang::BinaryOperatorKind opcode) {
        switch (opcode) {
            case clang::BO_Add: return "+";
            case clang::BO_Sub: return "-";
            case clang::BO_Mul: return "*";
            case clang::BO_Div: return "/"; 
            case clang::BO_Rem: return "%";
            case clang::BO_LT: return "<";
            case clang::BO_GT: return ">";
            case clang::BO_LE: return "<=";
            case clang::BO_GE: return ">=";
            case clang::BO_EQ: return "==";
            case clang::BO_NE: return "!=";
            case clang::BO_And: return "&";
            case clang::BO_Or: return "|";
            case clang::BO_Xor: return "^";
            case clang::BO_Shl: return "<<";
            case clang::BO_Shr: return ">>";
            default: return "?";
        }
    }

};


//make it global
std::unordered_map<std::string, DefinitionInfo> varDefs;

int getCurrentSsaVersionOfVariable(string baseName){
    int currentVersion = -1;
    for (const auto& [ssaName, def] : varDefs) {
        if (def.ssaVar.name == baseName && def.ssaVar.version > currentVersion)
            currentVersion = def.ssaVar.version;
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

struct PathCondition {
    const clang::Expr* conditionExpr = nullptr;  // Initialize pointer!
    bool isTrueBranch = false;                   // Default to else branch
    std::string smtString;                       // Empty by default (std::string is safe)
    std::vector<std::string> ssaUsedVars;
    std::string smtStringWithSafetyAssertion;   
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
    std::string getPyz3smtStringExpression() const {
        return smtString;
    }
    std::string getPyz3smtStringExpressionWithSafetyAssertion() const {
        
        return smtStringWithSafetyAssertion;
    }
};


int DefinitionInfo::freshVarKcounter = 0;
int DefinitionInfo::freshVarXbarcounter = 0;
std::vector<std::string> DefinitionInfo::freshVarNames;

// Pure string utility: strip the "_N" SSA suffix from a name like "uid_sid_3".
// Returns the original string unchanged if it has no numeric suffix.
std::string getBaseNameFromSSA(const std::string& ssaName) {
    if (ssaName.empty()) return ssaName;
    size_t lastUnderscore = ssaName.find_last_of('_');
    if (lastUnderscore == std::string::npos || lastUnderscore == 0 ||
        lastUnderscore >= ssaName.length() - 1)
        return ssaName;
    std::string suffix = ssaName.substr(lastUnderscore + 1);
    for (char c : suffix)
        if (!std::isdigit(c)) return ssaName;
    return ssaName.substr(0, lastUnderscore);
}


class TargetProgramPoint {
    
private:
    std::unordered_map<unsigned, std::vector<DefinitionInfo>> lineToDefinitions;
    std::stack<std::string> conditionStack;  
public:
    const Stmt *globalTargetStmt;
    ASTContext *globalAstContext;
    const FunctionDecl *parentFunctionOfProgramPoint;
    std::set<std::string> variablesRelatedToTargetStmt;
    bool targetFound = false;
    std::vector<std::string> getCurrentPathConditions() const;
    std::set<std::string> relevantVariables;
    std::stack<PathCondition> gloablPathConditions;
    SmtScript pythonScript;


    TargetProgramPoint(){};
    TargetProgramPoint(const Stmt *targetStmt, ASTContext &astContext) {
        globalTargetStmt = targetStmt;
        globalAstContext = &astContext;
        sharedASTContext = &astContext;
        //cout << "from target " << globalTargetStmt->getID(*globalAstContext)<< "\n";
        //globalTargetStmt->dump();
    }



    void findParentStmt(const Stmt *s) {
        if (!s) return;
        const auto &parents = globalAstContext->getParents(*s);
        if (parents.empty()) return;
        const auto &firstParent = *parents.begin();
        if (const FunctionDecl *funcDecl = firstParent.get<FunctionDecl>()) {
            parentFunctionOfProgramPoint = funcDecl;
            return;
        }
        if (const Stmt *parentStmt = firstParent.get<Stmt>()) {
            findParentStmt(parentStmt);
        }
    }   


    bool VisitBinaryOperator(BinaryOperator* binOp, const Stmt* originalStmt = nullptr) {
        if (binOp->isAssignmentOp()) {
            Expr* lhs = binOp->getLHS()->IgnoreParenImpCasts();
            Expr* rhs = binOp->getRHS()->IgnoreParenImpCasts();

            if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(lhs)) {
                std::string varName = declRef->getNameInfo().getAsString();
                string varTypeStr = "";
                if (const VarDecl *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                    varTypeStr=(vd->getType()).getAsString();     
                }
                // Collect used variables (you'll need to pass current path state)
                std::vector<string> usedVars;
                collectUsedVars(rhs, usedVars);

                // Get current path conditions
                std::vector<std::string> currentPathConditions;
                std::stack<std::string> tmpStack = conditionStack;
                while (!tmpStack.empty()) {
                    currentPathConditions.insert(currentPathConditions.begin(), tmpStack.top());
                    tmpStack.pop();
                }

                // Safety net: if the variable has never been seen before and we are
                // inside a conditional, reserve version 0 as its symbolic initial value so
                // the conditional assignment gets version 1 (rollback = version 0, not -1).
                if (getCurrentSsaVersionOfVariable(varName) < 0 && !gloablPathConditions.empty()) {
                    SSAVariable initVar(varName, 0);
                    FullSourceLoc initLoc = globalAstContext->getFullLoc(binOp->getExprLoc());
                    DefinitionInfo initInfo(initVar, initLoc.getSpellingLineNumber(), 0,
                                           varTypeStr, "", {}, {}, nullptr);
                    initInfo.smtDefinitionExpression = "";
                    initInfo.isCondtionalDefinition = false;
                    varDefs[initVar.ssaName()] = initInfo;
                }

                SSAVariable newDef(varName);

                FullSourceLoc fullLoc = globalAstContext->getFullLoc(binOp->getExprLoc());
                unsigned line = fullLoc.getSpellingLineNumber();
                unsigned stmtID = reinterpret_cast<uintptr_t>(binOp);
                
                



                DefinitionInfo info(newDef,
                    line,
                    stmtID,
                    varTypeStr,
                    "",
                    usedVars,
                    currentPathConditions,
                    binOp);
                info.copyCondPathFromstackToVector(gloablPathConditions);
                
                lineToDefinitions[line].push_back(info);
                varDefs[newDef.ssaName()] = info;
                

            }
       
            //else{
            //    llvm::outs() << "DEBUG: Inside Assignment Binary Operator type not handled: \n";
            //    lhs->dump();
            //}
        }//else{
            //llvm::outs() << "DEBUG: VisitBinaryOperator statement type not handled: " <<  binOp->getOpcodeStr()  << "\n";
            //binOp->dump();
        //}
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

        // Register function parameters as version-0 free variables so that
        // exprToPyZ3 resolves them to "param_0" and they get declared in the script.
        for (const ParmVarDecl* param : func->parameters()) {
            std::string paramName = param->getNameAsString();
            if (paramName.empty() || getCurrentSsaVersionOfVariable(paramName) >= 0)
                continue;
            SSAVariable paramVar(paramName, 0);
            DefinitionInfo info(paramVar, 0, 0, param->getType().getAsString());
            info.isCondtionalDefinition = false;
            info.smtDefinitionExpression = "";
            varDefs[paramVar.ssaName()] = info;
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
        relevantVariables=computeRelevantVariablesWithSSANames();

        std::string script;
        script += "from z3 import *\n";
        script += "\n\n# *** Variables Declarations *** \n\n";
        script += genVariableDeclarationPyZ3();
        script += "\n\n# *** Variables Definitions *** \n\n";
        script += genVariableDefinitionPyZ3();
        script += "\n\n# *** Condition path *** \n\n";
        script += genConditionPathPyZ3();
        script += "\n\n# *** User constraints *** \n\n";
        script += genConstraintsPyZ3();
        script += "\n\n# *** Type range constraints *** \n\n";
        script += genTypeRangeConstraintsPyZ3();
        script += "\n\n# *** Checking satisfiability and getting models *** \n\n";
        script += "print(s.check())\n";
        script += "if(s.check()==sat):\n";
        script += "\t print(s.model())\n";

        // Write to checking.py
        std::ofstream pyFile("checking.py");
        if (pyFile.is_open()) {
            pyFile << script;
            pyFile.close();
            cout << "\n[*] Script written to checking.py\n";
        } else {
            cerr << "Error: could not write to checking.py\n";
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
            string condType="";
            DefinitionInfo condInfo = {
                condVar,
                line,
                stmtID,
                condType,
                condVar.ssaName() + " = " + rewrittenCond,
                usedVars,
                {},  // conditionContext will be filled from stack
                ifStmt    // defStmt
            };
            
            // Fill condition context from current stack
            std::stack<std::string> tmpStack = conditionStack;
            while (!tmpStack.empty()) {
                condInfo.conditionContext.insert(condInfo.conditionContext.begin(), tmpStack.top());
                tmpStack.pop();
            }
            condInfo.copyCondPathFromstackToVector(gloablPathConditions);

            lineToDefinitions[line].push_back(condInfo);
            varDefs[condVar.ssaName()] = condInfo;

            // Compute safety-annotated condition BEFORE any branch traversal
            // (same SSA state as rewrittenCond — avoids stale version lookups)
            std::string safetyCondText = DefinitionInfo::handleArithmeticWithOverflow(condExpr);

            if (const Stmt* thenBody = ifStmt->getThen()) {
                conditionStack.push(rewrittenCond);
                PathCondition pc(condExpr, true, usedVars);
                pc.smtString=rewrittenCond;
                pc.smtStringWithSafetyAssertion = safetyCondText;
                gloablPathConditions.push(pc);
                if (visitAllStmts(thenBody)) return true;
                conditionStack.pop();
                gloablPathConditions.pop();
            }

            if (const Stmt* elseBody = ifStmt->getElse()) {
                conditionStack.push("Not(" + rewrittenCond + ")");
                PathCondition pc(condExpr, false, usedVars);
                pc.smtString="Not(" + rewrittenCond + ")";
                pc.smtStringWithSafetyAssertion = "Not(" + safetyCondText + ")";
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

        // Register variable declarations (int a = 0; / int b;) as SSA version 0.
        // Without this, the first conditional assignment becomes version 0 and
        // its rollback version would be -1 (a_-1).
        if (isa<DeclStmt>(stmt)) {
            const DeclStmt* declStmt = cast<DeclStmt>(stmt);
            for (const Decl* decl : declStmt->decls()) {
                if (const VarDecl* vd = dyn_cast<VarDecl>(decl)) {
                    std::string varName = vd->getNameAsString();
                    if (getCurrentSsaVersionOfVariable(varName) >= 0) continue; // already registered

                    std::string varTypeStr = vd->getType().getAsString();
                    SSAVariable initVar(varName, 0);

                    FullSourceLoc fullLoc = globalAstContext->getFullLoc(vd->getLocation());
                    unsigned line = fullLoc.getSpellingLineNumber();
                    unsigned stmtID = reinterpret_cast<uintptr_t>(vd);

                    std::string smtExpr;
                    std::vector<std::string> usedVars;
                    if (vd->hasInit()) {
                        Expr* init = const_cast<Expr*>(vd->getInit()->IgnoreParenImpCasts());
                        collectUsedVars(init, usedVars);
                        smtExpr = DefinitionInfo::handleArithmeticWithOverflow(init);
                    }

                    DefinitionInfo info(initVar, line, stmtID, varTypeStr, "", usedVars, {}, nullptr);
                    info.smtDefinitionExpression = smtExpr;
                    info.isCondtionalDefinition = false; // VarDecl is unconditional in SSA
                    varDefs[initVar.ssaName()] = info;
                    lineToDefinitions[line].push_back(info);
                }
            }
            // Fall through to child iteration so targets inside initializers are still found
        }

        for (auto child = stmt->child_begin(); child != stmt->child_end(); ++child) {
            if (visitAllStmts(*child)) {
                return true;
            }
        }

        return false;
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
        }else if(const auto* funcCall = dyn_cast<clang::CallExpr>(globalTargetStmt)){
            for (unsigned i = 0; i < funcCall->getNumArgs(); ++i) {
                const clang::Expr* arg = funcCall->getArg(i);
                std::vector<std::string> argVars;
                extractVariablesFromExpr(arg, argVars);
                for (const auto& argBaseName : argVars) {
                        std::string argSSA = findLatestSSAName(argBaseName);
                        if (!argSSA.empty()) {
                            relevantSSANames.insert(argSSA);
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
                    //include used vars in cond path of its definition:
                    if(def.isCondtionalDefinition){
                        for (const auto& cond : def.globalConditionContext) {
                            for (const auto& var : cond.ssaUsedVars) {
                                if (relevantSSANames.insert(var).second) {
                                        changed = true;
                                }
                            }
                        }
                        // Include rollback variable (phi-node alternative: x_N-1 when condition is false)
                        if (def.ssaVar.version > 0) {
                            std::string rollback = def.ssaVar.name + "_" + std::to_string(def.ssaVar.version - 1);
                            if (relevantSSANames.insert(rollback).second) {
                                changed = true;
                            }
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
        return ::getBaseNameFromSSA(ssaName);
    }
    std::string findLatestSSAName(const std::string& baseName) const {
        int v = getCurrentSsaVersionOfVariable(baseName);
        if (v < 0) return "";
        return baseName + "_" + std::to_string(v);
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

    // Find the actual assignment statement if we have a literal
    const clang::Stmt* actualTarget = globalTargetStmt;

    // If target is a literal, try to find its parent assignment
    if (isa<clang::IntegerLiteral>(globalTargetStmt) ||
        isa<clang::FloatingLiteral>(globalTargetStmt) ||
        isa<clang::StringLiteral>(globalTargetStmt)) {

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
        context.targetVariables.push_back(dre->getNameInfo().getAsString());
    }
    else {
        // Try to extract variables anyway
        extractVariablesFromExpr(dyn_cast_or_null<clang::Expr>(actualTarget), context.targetVariables);
    }
    
    // Build variable definitions
    for (const auto& [ssaName, def] : varDefs) {
        SMTVariableDefinition smtDef;
        smtDef.baseName = def.ssaVar.name;
        smtDef.ssaName = def.ssaVar.ssaName();
        smtDef.version = def.ssaVar.version;
        smtDef.defStmt = def.defStmt;
        smtDef.exprString = def.exprString;
        smtDef.pathConditions = def.conditionContext;
        smtDef.line = def.line;
        smtDef.stmtID = def.stmtID;
        
        context.variableDefinitions[smtDef.baseName].push_back(smtDef);
    }
    
    // Compute relevant variables
    context.relevantVariables =  computeRelevantVariablesWithSSANames();
    
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
            info.pathConditions = def.conditionContext;
        }
        return info;
    }

    std::string genConditionPathPyZ3() {
        if (gloablPathConditions.empty()) {
            return "# No path conditions\n";
        }

        // Copy stack and reverse to get outermost condition first
        std::stack<PathCondition> temp = gloablPathConditions;
        std::vector<PathCondition> conditions;
        while (!temp.empty()) {
            conditions.push_back(temp.top());
            temp.pop();
        }
        std::reverse(conditions.begin(), conditions.end());

        std::string result = "s = Solver()\n";
        for (const auto& pc : conditions) {
            result += "s.add(" + pc.getPyz3smtStringExpressionWithSafetyAssertion() + ")\n";
        }
        return result;
    }

    std::string genConstraintsPyZ3() {
        if (sharedConstraints.empty()) return "";
        std::string result;
        for (const auto& uc : sharedConstraints) {
            std::string ssaName = findLatestSSAName(uc.variable);
            if (ssaName.empty()) ssaName = uc.variable;
            result += "s.add(" + ssaName + " " + uc.op + " " + uc.value + ")\n";
        }
        return result;
    }

    std::string genTypeRangeConstraintsPyZ3() {
        std::string result;
        std::set<std::string> seen; // one range per SSA name

        for (const auto& ssaName : relevantVariables) {
            if (seen.count(ssaName)) continue;
            seen.insert(ssaName);

            auto it = varDefs.find(ssaName);
            if (it == varDefs.end()) continue;

            const std::string& vt = it->second.varType;
            if (vt.empty()) continue;

            // Determine signedness and bit width from the type string
            bool isUnsigned = (vt.find("unsigned") != std::string::npos);

            unsigned bits = 0;
            if (vt.find("short")      != std::string::npos) bits = 16;
            else if (vt.find("long long") != std::string::npos) bits = 64;
            else if (vt.find("long")  != std::string::npos) bits = 64; // refined below
            else if (vt.find("int")   != std::string::npos) bits = 32;
            else if (vt.find("char")  != std::string::npos) bits = 8;

            if (bits == 0) continue;

            // Refine 'long' width via execution-environment rules (LP64 vs LLP64/ILP32)
            if (vt == "long" || vt == "unsigned long") {
                // Recompute via env rules
                bool longIs32 = false;
                auto toLow = [](std::string s){
                    std::transform(s.begin(),s.end(),s.begin(),::tolower); return s; };
                for (auto& f : sharedExecutionEnv.flags)
                    if (f == "-m32") { longIs32 = true; break; }
                std::string osLow = toLow(sharedExecutionEnv.os);
                if (osLow=="windows"||osLow=="win32"||osLow=="win64"||osLow=="win")
                    longIs32 = true;
                std::string archLow = toLow(sharedExecutionEnv.arch);
                if (archLow=="i386"||archLow=="i686"||archLow=="x86"||archLow=="arm")
                    longIs32 = true;
                bits = longIs32 ? 32 : 64;
            }

            std::string lo, hi;
            if (isUnsigned) {
                lo = "0";
                hi = "2**" + std::to_string(bits) + " - 1";
            } else {
                lo = "-2**" + std::to_string(bits - 1);
                hi = "2**"  + std::to_string(bits - 1) + " - 1";
            }
            result += "s.add(" + ssaName + " >= " + lo + ")\n";
            result += "s.add(" + ssaName + " <= " + hi + ")\n";
        }
        return result;
    }

    std::string  genVariableDeclarationPyZ3(){
        std::string fullDeclaration="";
        for (const auto& ssaName : relevantVariables) {
            auto it = varDefs.find(ssaName);
            if (it != varDefs.end()) {
                const DefinitionInfo& def = it->second;
                fullDeclaration=fullDeclaration+def.getPyZ3Declaration();
                pythonScript.pushIfNotInDeclarationVector(def.getPyZ3Declaration());
            }else{
                cout<<" didn't find ssa var in varDefs \n";
            }
            fullDeclaration=fullDeclaration+"\n";
        }
        // Declare fresh overflow variables (k1, xbar1, etc.) generated during definitions
        for (const auto& freshVar : DefinitionInfo::freshVarNames) {
            std::string decl = freshVar + " = Int('" + freshVar + "')";
            fullDeclaration = fullDeclaration + decl + "\n";
            pythonScript.pushIfNotInDeclarationVector(decl);
        }
        return fullDeclaration;
    }


std::string genVariableDefinitionPyZ3() {
    std::set<std::string> allVariables;
    std::unordered_map<std::string, std::set<std::string>> dependencies;
    
    // Collect all variables and their dependencies
    std::set<std::string> workSet = relevantVariables;
    
    // Keep expanding until we have all transitive dependencies
    bool changed = true;
    while (changed) {
        changed = false;
        std::set<std::string> newWorkSet = workSet;
        
        for (const auto& ssaName : workSet) {
            auto it = varDefs.find(ssaName);
            if (it != varDefs.end()) {
                allVariables.insert(ssaName);
                dependencies[ssaName] = std::set<std::string>();
                const DefinitionInfo& def = it->second;
                
                // Add usedVars dependencies
                for (const auto& usedVar : def.usedVars) {
                    if (!usedVar.empty() && usedVar != ssaName) {
                        dependencies[ssaName].insert(usedVar);
                        if (allVariables.find(usedVar) == allVariables.end()) {
                            newWorkSet.insert(usedVar);
                            changed = true;
                        }
                    }
                }
                
                // Add condition context dependencies  
                for (const auto& pathCond : def.globalConditionContext) {
                    for (const auto& condVar : pathCond.ssaUsedVars) {
                        if (!condVar.empty() && condVar != ssaName) {
                            dependencies[ssaName].insert(condVar);
                            if (allVariables.find(condVar) == allVariables.end()) {
                                newWorkSet.insert(condVar);
                                changed = true;
                            }
                        }
                    }
                }
                
                // Add rollback dependency
                if (def.isCondtionalDefinition && def.ssaVar.version > 0) {
                    std::string rollback = def.ssaVar.name + "_" + std::to_string(def.ssaVar.version - 1);
                    dependencies[ssaName].insert(rollback);
                    if (allVariables.find(rollback) == allVariables.end()) {
                        newWorkSet.insert(rollback);
                        changed = true;
                    }
                }
            }
        }
        workSet = newWorkSet;
    }
    
    // Ensure all variables have dependency entries
    for (const auto& var : allVariables) {
        if (dependencies.find(var) == dependencies.end()) {
            dependencies[var] = std::set<std::string>();
        }
    }
    
    // Simple iterative approach: keep emitting variables whose dependencies are satisfied
    std::set<std::string> emitted;
    std::vector<std::string> sortedOrder;
    
    bool progress = true;
    while (progress && emitted.size() < allVariables.size()) {
        progress = false;
        for (const auto& var : allVariables) {
            if (emitted.find(var) != emitted.end()) continue;
            
            // Check if all dependencies are already emitted
            bool canEmit = true;
            for (const auto& dep : dependencies[var]) {
                if (allVariables.count(dep) && emitted.find(dep) == emitted.end()) {
                    canEmit = false;
                    break;
                }
            }
            
            if (canEmit) {
                sortedOrder.push_back(var);
                emitted.insert(var);
                progress = true;
            }
        }
    }
    
    // Generate definitions
    std::string fullDefinition = "";
    for (const auto& ssaName : sortedOrder) {
        auto it = varDefs.find(ssaName);
        if (it != varDefs.end()) {
            const DefinitionInfo& def = it->second;
            std::string condExpression = "";
            
            if (!def.isCondtionalDefinition) {
                // Skip uninitialized declarations (int b;) — free symbolic variable, no RHS
                if (!def.smtDefinitionExpression.empty()) {
                    fullDefinition += ssaName + " = " + def.smtDefinitionExpression + "\n";
                }
            } else {
                if (def.globalConditionContext.size() > 1) {
                    condExpression = "And( ";
                    for (size_t i = 0; i < def.globalConditionContext.size(); i++) {
                        if (i > 0) condExpression += ", ";
                        condExpression += def.globalConditionContext[i].getPyz3smtStringExpressionWithSafetyAssertion();
                    }
                    condExpression += " )";
                } else if (def.globalConditionContext.size() == 1) {
                    condExpression = def.globalConditionContext[0].getPyz3smtStringExpressionWithSafetyAssertion();
                }
                
                std::string rollBackSsaName = def.ssaVar.name + "_" + std::to_string(def.ssaVar.version - 1);
                fullDefinition += ssaName + " = If( " + condExpression + ", " + def.smtDefinitionExpression + ", " + rollBackSsaName + ")\n";
            }
        }
    }
    
    return fullDefinition;
}


};

void SmtScript::pushIfNotInDeclarationVector(const std::string& decl) {
    for (const auto& existing : variableDeclarations) {
        if (existing == decl) {
            return;
        }
    }
    variableDeclarations.push_back(decl);
}

void SmtScript::pushIfNotInDefinitionVector(const std::string& def) {
    for (const auto& existing : variableDefinitions) {
        if (existing == def) {
            return;
        }
    }
    variableDefinitions.push_back(def);
}

void SmtScript::printVariableDeclarations() {
    llvm::outs() << "\n from z3 import *\n\n";
    llvm::outs() << "\n# *** Variables Declarations ***\n\n";
    for (const auto& decl : variableDeclarations) {
        llvm::outs() << decl << "\n";
    }
}

void SmtScript::printVariableDefinitions() {
    llvm::outs() << "\n# *** Variables Definitions ***\n\n";
    for (const auto& def : variableDefinitions) {
        llvm::outs() << def << "\n";
    }
}

// Generate fresh variable with unique counter
std::string SmtScript::generateFreshVar(const std::string& baseName) {
    int& counter = freshVarCounters[baseName];
    counter++;
    std::string freshVar = baseName + "_" + std::to_string(counter);
    return freshVar;
}

// Ensure variable is declared (and add declaration if not)
void SmtScript::ensureVariableDeclared(const std::string& varName, const std::string& type) {
    // Check if already declared
    bool alreadyDeclared = false;
    for (const auto& decl : variableDeclarations) {
        if (decl.find(varName + " = ") == 0) {
            alreadyDeclared = true;
            break;
        }
    }
    
    if (!alreadyDeclared) {
        std::string decl = varName + " = " + type + "('" + varName + "')";
        variableDeclarations.push_back(decl);
    }
}