#pragma once
#include "main.h"



struct SSAVariable {
    std::string name;
    int version;
    std::string ssaName() const { return name + "_" + std::to_string(version); }
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
                rhs->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy());
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

                FullSourceLoc fullLoc = globalAstContext->getFullLoc(binOp->getExprLoc());
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


  void exploreFunctionByStmt(){
    
    const FunctionDecl* func = parentFunctionOfProgramPoint;
    if (!func || !func->hasBody()) {
        return;
    }
    const Stmt* body = func->getBody();  // ← This is a Stmt*
    // Now iterate over all child statements recursively
    visitAllStmts(body);
    printVariableHistory();

    //parentFunctionOfProgramPoint->dump();
    
  }
  bool visitAllStmts(const Stmt* stmt) {
    Stmt* nonConstStmt = const_cast<clang::Stmt*>(stmt);
    if (!stmt) return false;

    // Process this statement
    //llvm::outs() << "Visiting: " << stmt->getStmtClassName() << "\n";
    if (stmt == globalTargetStmt) {

        llvm::outs() << "******* TARGET STMT FOUND — STOPPING \n";
        printConditionStackTopToBottom(conditionStack);
        return true;
    }

    if(isa<IfStmt>(nonConstStmt)){
      IfStmt *ifStmt = cast<IfStmt>(nonConstStmt);
      //VisitIfStmt(ifStmt);
      Expr* condExpr = ifStmt->getCond();
      std::vector<SSAVariable> usedVars;
      collectUsedVars(condExpr, usedVars);

      std::string condText;
      llvm::raw_string_ostream ss(condText);
      condExpr->printPretty(ss, nullptr, globalAstContext->getPrintingPolicy());
      std::string rewrittenCond = rewriteWithSSA(ss.str(), usedVars);

      SSAVariable condVar = symtab.define("cond"); // pseudo SSA condition var
      FullSourceLoc fullLoc = globalAstContext->getFullLoc(ifStmt->getIfLoc());
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
      //conditionStack.push(rewrittenCond);
      if (const Stmt* thenBody = ifStmt->getThen()) {
          conditionStack.push(rewrittenCond);           // positive condition
          if (visitAllStmts(thenBody)) return true;
          conditionStack.pop();
      }

      // Visit ELSE branch
      if (const Stmt* elseBody = ifStmt->getElse()) {
          conditionStack.push("!(" + rewrittenCond + ")"); // negated condition
          if (visitAllStmts(elseBody)) return true;
          conditionStack.pop();
      }
      return false; // already recursed into children

    }
    if(isa<BinaryOperator>(nonConstStmt)){
        BinaryOperator *binOp = cast<BinaryOperator>(nonConstStmt);
        VisitBinaryOperator(binOp);
    }
    
    // Recurse into children — stop if any child finds the target
    for (auto child = stmt->child_begin(); child != stmt->child_end(); ++child) {
        if (visitAllStmts(*child)) {
            return true; // propagate "found" upward
        }
    }

    return false; // not found in this subtree
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
  void findParentStmtOld(const Stmt *s, bool allParentsFound) {
    /*currentCallCounter++;
    logFile<<" starting findparent CALL COUNTER :# "<< currentCallCounter<<"\n";*/
    if (allParentsFound) {
      cout << "all parent found : " << vectorOfParentStmt.size() << "\n";
    }else {
      const Stmt *ST = s;
      //ST->dump();
      const auto &parents = globalAstContext->getParents(*ST);
      //cout<<"findparent after &parents\n";
      if (parents.empty()) {
        llvm::errs() << "Can not find parent\n";
        allParentsFound = true;

      }else {
          auto it = globalAstContext->getParents(*ST).begin();
          // replacing llvm::outs() with logFile to cleanun up console logFile << 
          llvm::outs() << "Current Node Kind: " << it->getNodeKind().asStringRef()<< "\n";
          if(it->getNodeKind().asStringRef()=="FunctionDecl"){
              //parentFunctionOfProgramPoint=
                parentFunctionOfProgramPoint=parents[0].get<FunctionDecl>();
                //parentFunctionOfProgramPoint->dump();
          }
          if (parents.size() == 1) {
            ST = parents[0].get<Stmt>();
            if (!ST) {
              // llvm::outs() << "Can not find parent sec block\n";
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
      /*else {
        // TODO: handle nodes with more than 1 direct parents
      }*/
    }
  }

  void getConditionPath() {
    //cout<<"running getConditionPath here\n";
    ASTContext *astContext = globalAstContext;

    const SourceManager &srcMgr = astContext->getSourceManager();

    const Expr *condition;
    std::string expression = "";
    std::vector<std::string> allExpressions = {};

    std::string condState = "";
    for (unsigned int i = 0; i < vectorOfParentStmt.size(); ++i) {
      //cout<<" for loop getConditionPath index "<<i<<" \n";
      const Stmt *st = vectorOfParentStmt[i].st;
      if (isa<IfStmt>(st)) {
        

        const IfStmt *ifStmt = cast<IfStmt>(st);
        //cout<<"starting for ifStmt getConditionPath here with Id= "<<(ifStmt->getThen())->getID(*globalAstContext) <<"\n";
        if (i - 1 < vectorOfParentStmt.size()) {
          StmtAttributes nextStmt = vectorOfParentStmt[i - 1];
          if (nextStmt.id == (ifStmt->getThen())->getID(*globalAstContext)) {
            //cout<<"vectorSize = "<<vectorOfParentStmt.size()<<" access element vector[ "<<i - 1 <<" ]\n";
            condState = "";
            listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));

          }
          if(ifStmt->getElse()){
            if (nextStmt.id == (ifStmt->getElse())->getID(*globalAstContext)) {
                condState = "Not";
                listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not"));
           }
       
       

          }
          
        }
        //cout<<"conditionnnnn here\n";
        condition = ifStmt->getCond();
        //condition->dump();
        const Stmt *conditionStmt= ifStmt->getCond(); 
        
        //listOfAtomicElements.push_back(AtomicElementOfConditionPath("binOp",binOpStmt));
        //buildSemanticOfConditionPath(condition);


        allExpressions.push_back(
            condState + "( " +  binaryOpToStr(conditionStmt, srcMgr, globalAstContext)  + " )");
        //cout<<"allexpression size"<< allExpressions.size()<<"\n";
      } else if (isa<ForStmt>(st)) {

        const ForStmt *forStmt = cast<ForStmt>(st);
        condition = forStmt->getCond();
        const Stmt *conditionStmt= forStmt->getCond(); 

        //condition->dump();
        if(condition){
          FullSourceLoc startLocation =astContext->getFullLoc(condition->getBeginLoc());
          SourceRange sr = condition->getSourceRange();
          /* allExpressions.push_back(
            "( " + std::string(get_source_text(sr, srcMgr)) + " )"); */
          listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));
          allExpressions.push_back(
             "( " +  binaryOpToStr(conditionStmt, srcMgr, globalAstContext)  + " )");  

        }else{
          allExpressions.push_back("( TRUE )");

        }
        

      } else if (isa<WhileStmt>(st)) {

        const WhileStmt *whileStmt = cast<WhileStmt>(st);
        condition = whileStmt->getCond();
        const Stmt *conditionStmt= whileStmt->getCond(); 
        //condition->dump();
        if(condition){
          FullSourceLoc startLocation =astContext->getFullLoc(condition->getBeginLoc());
          SourceRange sr = condition->getSourceRange();
          /* allExpressions.push_back(
            "( " + std::string(get_source_text(sr, srcMgr)) + " )"); */
          listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));
          allExpressions.push_back(
             "( " +  binaryOpToStr(conditionStmt, srcMgr, globalAstContext)  + " )");  


        }else{
          allExpressions.push_back("( TRUE )");

        }

      } else if (isa<ConditionalOperator>(st)) {

        const ConditionalOperator *cdOp = cast<ConditionalOperator>(st);
        condition = cdOp->getCond();
        FullSourceLoc startLocation =
            astContext->getFullLoc(condition->getBeginLoc());
        SourceRange sr = condition->getSourceRange();
        allExpressions.push_back(
            "( " + std::string(get_source_text(sr, srcMgr)) + " )");
      } else if (isa<DoStmt>(st)) {

        const DoStmt *doStmt = cast<DoStmt>(st);
        condition = doStmt->getCond();
        const Stmt *conditionStmt= doStmt->getCond(); 

         //condition->dump();
        if(condition){
          FullSourceLoc startLocation =astContext->getFullLoc(condition->getBeginLoc());
          SourceRange sr = condition->getSourceRange();
          //allExpressions.push_back(
          //  "( " + std::string(get_source_text(sr, srcMgr)) + " )");
          listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));
          allExpressions.push_back(
            "( " + binaryOpToStr(conditionStmt, srcMgr, globalAstContext) + " )");
        }else{
          allExpressions.push_back("( TRUE )");

        }
      } /*else if (isa<SwitchStmt>(s)) {
           child = ((SwitchStmt*) s)->getSwitchCaseList();
           while (child != NULL) {
               if (isa<DefaultStmt>(child)) {
               hasDefault = true;
               break;
               }
               child = child->getNextSwitchCase();
           }

           if (!hasDefault)  {
               printInfo("ImpDef", lineNum, colNum, fileName);
               this->branches += 1;
               SwitchStmt *switchStmt = cast<SwitchStmt>(s);
               condition = switchStmt->getCond();

           }
       }else if (isa<CaseStmt>(s))  {
           printInfo("Case", lineNum, colNum, fileName);
           this->branches += 1;
           CaseStmt *caseStmt = cast<CaseStmt>(s);

       }else if (isa<DefaultStmt>(s))  {
           printInfo("Default", lineNum, colNum, fileName);
           this->branches += 1;
           DefaultStmt *defaultStmt = cast<DefaultStmt>(s);

       } */
     
     
       //cout<<"endLoop round \n";
     }
    if (allExpressions.size() == 0) {
      cout << "the target instruction will be executed at each call of "<<
      parentFunctionOfProgramPoint->getNameInfo().getName().getAsString()
              <<" function \n";
    } else {

      for (unsigned int i = 0; i < allExpressions.size() - 1; i++) {
        expression = expression + allExpressions[i] + " and ";
      }
      cout << expression + allExpressions[allExpressions.size() - 1] << "\n";
    }

    // cout<<"end of confition path size vec
    // "<<vectorOfParentStmt.size()<<"\n";
  }


};
