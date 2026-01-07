#pragma once
#include "main.h"




class TargetProgramPoint {
public:
  const Stmt *globalTargetStmt;
  ASTContext *globalAstContext;
  std::vector<StmtAttributes> vectorOfParentStmt = {};
  const FunctionDecl *parentFunctionOfProgramPoint;
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


  void getConditionPathV2() {
    cout<<"running getConditionPathV2 here\n";
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

        //checking condition state disable comment after mapping new nodes of dataStructure
        
        if ( vectorOfParentStmt.size()> (i - 1) ) {
          StmtAttributes nextStmt = vectorOfParentStmt[i - 1];
          if (nextStmt.id == (ifStmt->getThen())->getID(*globalAstContext)) {
            // comment line 260 & 266
            cout<<"vectorSize = "<<vectorOfParentStmt.size()<<" access element vector[ "<<i - 1 <<" ]\n";
            condState = "";
            //listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not Not"));

          }
          if(ifStmt->getElse()){
            if(nextStmt.id == (ifStmt->getElse())->getID(*globalAstContext)) {
                condState = "Not";
                //listOfAtomicElements.push_back(AtomicElementOfConditionPath("unaryOp","Not"));
            }
          }
          
        } // check if we are in the block of then or in the else block
       
        condition = ifStmt->getCond();
     
        const Stmt *conditionStmt= ifStmt->getCond(); 
        if(condState=="Not"){
          NodeTool *tree = new NodeTool(conditionStmt);
          NodeTool root(condState,tree);
          cout<<"roooooooooooooot Not \n"<<root.pyz3ApiFlatten()<<"\n";
        }else{
          //NodeTool *tree = new NodeTool(conditionStmt); /* to avoid ==10661==ERROR: LeakSanitizer: detected memory leaks Direct leak of 96 byte(s) in 1 object(s) allocated from:    1 0x5d81cfdda85a in TargetProgramPoint::getConditionPathV2() src/TargetProgramPoint.cpp:315 */
          NodeTool tree(conditionStmt);
          cout<<"roooooooooooooot true \n"<<tree.pyz3ApiFlatten()<<"\n";
          
        }
        
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
