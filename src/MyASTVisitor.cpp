#pragma once
#include "main.h"


//map<TrackedVariable*, vector<DefinitionContext*>> mapOfVariablesWithDefContext;

string getCurrentSsaLabelOfVariable(VarDecl *VD);
class NodeExpression{
  
public:
    const Stmt *dataNodeElement;
    NodeExpression *leftNode;
    NodeExpression *rightNode; // set this node if unary
    vector<NodeTool *> definitionContext;
   
      
    bool isBinaryNode;
    bool isUnaryNode;
    bool isFakeNode;
    bool isLeafNode;
    bool isConditional;
    string typeNode;
    string typeLeafNode;


    NodeExpression(string typenode,NodeExpression *node){ //default constructor
        isFakeNode=true;
        dataNodeElement=nullptr;
        typeNode=typenode;
        rightNode=node;

      

    }
    NodeExpression(const Stmt *clangStmt,stack<NodeTool*> *myStack=nullptr ){
        cout<<"NodeExpression cnstructor\n";
        //clangStmt->dump();
        dataNodeElement=clangStmt;
        if(myStack){


          if(myStack->empty()){
            isConditional=false;
            definitionContext={};
          }else{
            isConditional=true;
            stack<NodeTool*> stackCopy(*myStack);
            while( !stackCopy.empty() ){
              definitionContext.push_back(stackCopy.top());
              stackCopy.pop(); 
            }
          }
        }else{
          isConditional=false;
          definitionContext={};

        }
        if(dyn_cast<BinaryOperator>(clangStmt)){
            //clangStmt->dump();
            isBinaryNode=true;
            isUnaryNode=false;
            isFakeNode=false;
            isLeafNode=false;

            const BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(clangStmt);
            //cout<<"New node "<<clangStmt->getStmtClassName() << " "<< std::string(BinOpStmt->getOpcodeStr())<<"\n";
          
            Expr *lhs = (BinOpStmt->getLHS());
            const clang::Stmt *leftStmt=dyn_cast<Stmt>(lhs);
            NodeExpression *newNodeLeft= new NodeExpression(leftStmt);
            leftNode= newNodeLeft;
            Expr *rhs = (BinOpStmt->getRHS());
            const clang::Stmt *rightStmt=dyn_cast<Stmt>(rhs);
            NodeExpression *newNoderight= new NodeExpression(rightStmt);
            rightNode=newNoderight;

        }else{
            leftNode=nullptr;  

            if (dyn_cast<ParenExpr>(clangStmt)){
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                const ParenExpr *parenExpr= dyn_cast<ParenExpr>(clangStmt);
                
                const clang::Stmt *subStmt= dyn_cast<Stmt> (parenExpr->getSubExpr());
                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                NodeExpression *newNoderight= new NodeExpression(subStmt);
                rightNode=newNoderight;
            }else if(isa<UnaryOperator>(clangStmt)){ //logical not 
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;
                //cout<<" &&&&&&&&&&&&&&& UNARY operator &&&&&&&&&&&&&";
                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                const UnaryOperator *unaryOp=dyn_cast<UnaryOperator>(clangStmt);
                const clang::Stmt *subStmt= dyn_cast<Stmt> (unaryOp->getSubExpr());
                //cout<<"\t"<< subStmt->getStmtClassName();   
                NodeExpression *newNoderight= new NodeExpression(subStmt);
                rightNode=newNoderight;

                
                
            }else if(isa<ImplicitCastExpr>(clangStmt)){ //implCast
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                const Expr *impCastExpr=(dyn_cast<CastExpr>(clangStmt))->getSubExpr();
                const clang::Stmt *subStmt= dyn_cast<Stmt> (impCastExpr);
                NodeExpression *newNoderight= new NodeExpression(subStmt);
                rightNode=newNoderight; 
            
             //CStyleCastExpr   
            }else if(isa<CStyleCastExpr>(clangStmt)){ //implCast
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                const Expr *castExpr=(dyn_cast<CStyleCastExpr>(clangStmt))->getSubExpr();
                const clang::Stmt *subStmt= dyn_cast<Stmt> (castExpr);
                NodeExpression *newNoderight= new NodeExpression(subStmt);
                rightNode=newNoderight; 
            
               
            }else if((isa<DeclRefExpr>(clangStmt))){

                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=true;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                typeLeafNode="var";
                const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(clangStmt);
                if (const VarDecl *vd = dyn_cast<VarDecl>(dre->getDecl())) {
                
                    TrackedVariables leftVar = TrackedVariables(vd->getNameAsString(), (vd->getType()).getAsString(),  vd);
                    typeLeafNode=  vd->getNameAsString()+getCurrentSsaLabelOfVariable(vd);  
                    //pushIfNotInList(leftVar);
                    //cout<<typeLeafNode<<" |||||||||||||\n";                              
                }
                
            }else if(isa<IntegerLiteral>(clangStmt)){

                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=true;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                typeLeafNode="lit";
                const IntegerLiteral *intLet= dyn_cast<IntegerLiteral>(clangStmt);
                FullSourceLoc startLocation =
                    sharedASTContext->getFullLoc(intLet->getBeginLoc());
                SourceRange sr = intLet->getSourceRange();
                const SourceManager &srcMgr = sharedASTContext->getSourceManager();
                typeLeafNode=std::string(get_source_text(sr, srcMgr)) ;
                //cout<<typeLeafNode<<" |||||||||||||\n";
                
            }else{
              cout<<"\nNOT handled AST clang node: Class "<<clangStmt->getStmtClassName()<<"\n";
            }
        }

    }


    string pyz3ApiFlatten(){
        pyz3ApiCallCounter++;
        
        string outputStr="";
        string conitionalStmt="";
        if(dataNodeElement){
            if(isConditional){
              for(NodeTool *cond : definitionContext){
                  conitionalStmt=conitionalStmt+cond->pyz3ApiFlatten()+ ", ";
              }
            }



            if(isBinaryNode){
                //cout<< "z3 call counter bina:  "<<pyz3ApiCallCounter<<"\n";
                const BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(dataNodeElement);
        
                string rightStr="",leftStr="",opCodeStr="";
                if(leftNode){

                    leftStr=leftNode->pyz3ApiFlatten();
                }
                if(rightNode){

                    rightStr=rightNode->pyz3ApiFlatten();
                }
                opCodeStr=std::string(BinOpStmt->getOpcodeStr());
                
                //check assgn op if defintioncontext is conditional
                if(opCodeStr =="=" && isConditional){
                  opCodeStr="==";
                  outputStr = leftStr +" "+ opCodeStr+" "+ rightStr+ " //if ";//+ conitionalStmt;
                }else{

                outputStr = leftStr +" "+ opCodeStr+" "+ rightStr;
                }
                



            }else if(isUnaryNode){
                //cout<< "z3 call counter una:  "<<pyz3ApiCallCounter<<"\n";
                if (dyn_cast<ParenExpr>(dataNodeElement)){
                    outputStr="( "+ rightNode->pyz3ApiFlatten() + " )\n";
                }else if(isa<UnaryOperator>(dataNodeElement)){  //check negat
                    const UnaryOperator *unaryOp=dyn_cast<UnaryOperator>(dataNodeElement);
                    if(std::string(unaryOp->getOpcodeStr(unaryOp->getOpcode()))=="!"){
                        
                        outputStr="Not( "+ rightNode->pyz3ApiFlatten() + " )\n";
                    }else{
                      //cout<< "Pyz3flaten unary else node "<<std::string(unaryOp->getOpcodeStr(unaryOp->getOpcode()))<<"\n";
                      if(std::string(unaryOp->getOpcodeStr(unaryOp->getOpcode()))=="-"){
                        
                        outputStr="-"+ rightNode->pyz3ApiFlatten() + " ";
                      }
                    }
                }else if(isa<IntegerLiteral>(dataNodeElement)){
                    outputStr=typeLeafNode;

                }else if(isa<DeclRefExpr>(dataNodeElement)){
                    outputStr=typeLeafNode;

                }else{
                    outputStr=rightNode->pyz3ApiFlatten();
                }



            }

        }else{         // fakeNode
            if(isFakeNode){
                if(typeNode=="Not"){
                    outputStr="Not( "+ rightNode->pyz3ApiFlatten()+ " )";
                }

            }else{
                outputStr="Oops";
            }
            
        }   
        return outputStr;
    }

    



};


//test C++ header file

#include "DefinitionContext.h"

DefinitionContext::DefinitionContext(NodeExpression *def, stack<NodeTool*> *myStack){

    fullDefinition=def;
    if(myStack->empty()){
      isConditional=false;
    }else{
      isConditional=true;
      stack<NodeTool*> stackCopy(*myStack);
      while( !stackCopy.empty() ){
        listOfConditions.push_back(stackCopy.top());
        stackCopy.pop(); 
      }
      

    }

    //cout<<"\n@@@@@@@@@@@\n"<<this->getDefinitionExpression()<<"\n";
}

string DefinitionContext::getDefinitionExpression(){
    string definitionExpr= (fullDefinition->rightNode)->pyz3ApiFlatten(); 

    string conditionExpression="";
    if(isConditional){
       
      for(uint i=0;i<listOfConditions.size();i++){
        conditionExpression=conditionExpression+listOfConditions[i]->pyz3ApiFlatten()+ " ";
      }
    }
    return definitionExpr ;//+" If( "+conditionExpression+" )\t" ;

 
  }
  string DefinitionContext::getConditionalExpression(){
    string conditionExpression="";
    if(isConditional){
       
      for(uint i=0;i<listOfConditions.size();i++){
        conditionExpression=conditionExpression+listOfConditions[i]->pyz3ApiFlatten()+ " ";
      }
    }
    return conditionExpression;
  }
  int  DefinitionContext::getSsaLabel(){
    return this->ssaLabel;

  }
  void DefinitionContext::setSsaLabel(int index){
    //cout<<"\n Call ssa setLabel \n";
    this->ssaLabel=index;
  }

/*
void updateVariablesWithContextDefinition(DefinitionContext *defCtx,VarDecl *VD){
    currentCallCounter++;
    logFile<<" starting updateVariablesWithContextDefinition CALL COUNTER :# "<< currentCallCounter<<"\n";
    auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it != AllTrackedVariables.end()) {
          int newSsaLableOf= it->vectorOfDefinitionContext.size();
          defCtx->setSsaLabel(newSsaLableOf);
          (it->vectorOfDefinitionContext).push_back(defCtx);
          
          cout<<"\n////////////// NUMBER of definitions '"<< it->varName<<"' is : "<< (it->vectorOfDefinitionContext).size()<<"\n";
          it->getCurrentDefinition();
        }else{
          cout<<"@@@@@@@@@@@   whoops else of updateVariablesWithContextDefinition maybe the variables is not tracked yet ! \n";

        }  
    
}
*/

string repeatStr(string sequence, int repaeatFactor){
  string newSequence="";
  for(int i=0;i<repaeatFactor;i++){
      newSequence=newSequence+sequence;
  }
  return newSequence;


}
void printStackContent(stack<int64_t> *stackParentBlocId){
  
  stack<int64_t> stackCopy(*stackParentBlocId);


  while( !stackCopy.empty() ) {
     //cout << stackCopy.top() << " ";
     stackCopy.pop(); 
  }
  //cout<<"\n"<<"END stack\n";

}



void printConditionalStackContent(stack<NodeTool*> *myStack){
  
  stack<NodeTool*> stackCopy(*myStack);


  while( !stackCopy.empty() ) {
     cout << (stackCopy.top())->pyz3ApiFlatten() << ", ";
     stackCopy.pop(); 
  }
  //cout<<"\n"<<"END conditional stack\n";

}



NodeTool* getConditionalStackContent(stack<NodeTool*> *myStack){
  
  stack<NodeTool*> stackCopy(*myStack);
  

  while( !stackCopy.empty() ) {
     cout << (stackCopy.top())->pyz3ApiFlatten() << ", ";
     stackCopy.pop(); 
  }
  //cout<<"\n"<<"END conditional stack\n";

}


stack<int64_t> *stackParentBlocId=new stack<int64_t> ();
stack<NodeTool* > stackCondiotionaltBloc;

void getVariablesUsedInExpression(Expr *expr){
  currentCallCounter++;
  logFile<<" starting getVariablesUsedInExpression CALL COUNTER :# "<< currentCallCounter<<"\n";
        
  if(isa<DeclRefExpr>(expr)){
    DeclRefExpr *dre = dyn_cast<DeclRefExpr>(expr);
    if (VarDecl *VD = dyn_cast<VarDecl>(dre->getDecl())) {
       
        auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it != AllTrackedVariables.end()) {
          
          cout<<"\n@@@@@ VARIABLE USE '"<< it->varName<<"' # is : "<< (it->vectorOfDefinitionContext).size()<<"\n";
          //it->getCurrentDefinition();
        }//else not 
    
    }
  }else if(isa<ParenExpr>(expr)){
    ParenExpr *parenExpr= dyn_cast<ParenExpr>(expr);
    if(isa<BinaryOperator>(dyn_cast<BinaryOperator>(parenExpr->getSubExpr()))){
      BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
      //call extract
      getVariablesUsedInExpression(dyn_cast<Expr>(binOpStmt));
      //cout<<"  extractUsedVariables : " <<(dyn_cast<Stmt>(binOpStmt))->getStmtClassName()<<"\n";

    }
      
  }else if(isa<BinaryOperator>(expr)){
    //cout<<"00000000000000000000000000000000000000000000000\n";
    BinaryOperator *binOpStmt=dyn_cast<BinaryOperator>(expr);
    //binOpStmt->dump();
    Expr *lhs = (binOpStmt->getLHS())->IgnoreImpCasts();
    //lhs->dump();
      //cout<<" initialize rhs\n";
    Expr *rhs = (binOpStmt->getRHS())->IgnoreImpCasts();
    //rhs->dump();
    getVariablesUsedInExpression(rhs);
    getVariablesUsedInExpression(lhs);
    
      
  }else if(isa<ImplicitCastExpr>(expr)){
    Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
    getVariablesUsedInExpression(SkipingImplicitCast);
  
  }else if(isa<CStyleCastExpr>(expr)){
    Expr *subExpr= (dyn_cast<CStyleCastExpr>(expr))->getSubExpr();
    getVariablesUsedInExpression(subExpr);
  
  }else if(isa<MemberExpr>(expr)){
    Expr *SkipingImplicitCast= (dyn_cast<ImplicitCastExpr>(expr))->IgnoreImpCasts();
    //extractUsedVariables(SkipingImplicitCast);   //try to replace extractUsedVariables by  getVariablesUsedInExpression
    getVariablesUsedInExpression(SkipingImplicitCast);
  
  }else{/*
    logFile<<" extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";
    cout<<" extractUsedVariables : " <<(dyn_cast<Stmt>(expr))->getStmtClassName()<<"\n";
    //expr->dump();
    cout<<"not decl ref\n";*/
  }
}



void exploreChildren(Stmt *stmt, int dept){
  
  
  
  string tab="\t";
  int64_t trueBlockId;
  int64_t falseBlockId;
  NodeTool *currentCond;
  if (isa<IfStmt>(stmt)) {
        const IfStmt *ifStmt = cast<IfStmt>(stmt);
        trueBlockId =(ifStmt->getThen())->getID(*sharedASTContext);
        falseBlockId =(ifStmt->getElse())->getID(*sharedASTContext);

        const Stmt *condiotnStmt=  dyn_cast<Stmt>(ifStmt->getCond());
        currentCond = new NodeTool(condiotnStmt);
        
  }


    for (auto child0 = stmt->child_begin(), e = stmt->child_end(); child0 != e; ++child0) {

        Stmt *childStmt = dyn_cast<Stmt>(*child0);
        if(childStmt->getID(*sharedASTContext)==trueBlockId ){
          stackParentBlocId->push(childStmt->getID(*sharedASTContext));
          stackCondiotionaltBloc.push(currentCond);

        }else if( childStmt->getID(*sharedASTContext)==falseBlockId ){
          stackParentBlocId->push(childStmt->getID(*sharedASTContext));
          NodeTool *notCondNode= new NodeTool("Not",currentCond);

          stackCondiotionaltBloc.push(notCondNode);

        }




        /*if(dyn_cast<BinaryOperator>(*child0)){
          //cout<<repeatStr(tab,dept)<<"-"<<child0->getStmtClassName()<< " "<< childStmt->getID(*sharedASTContext) <<std::string((dyn_cast<BinaryOperator>(*child0))->getOpcodeStr())<<"\n";
          }else{
          if (isa<IfStmt>(childStmt)) {
            const IfStmt *ifStmtChild = cast<IfStmt>(childStmt);
            int64_t trueBlockChild =(ifStmtChild->getThen())->getID(*sharedASTContext);
            int64_t falseBlockChild =(ifStmtChild->getElse())->getID(*sharedASTContext);
           // cout<< "getThen id "<< trueBlockChild << "getElse id "<< falseBlockChild;
          }


          //cout<<repeatStr(tab,dept)<<"-"<<child0->getStmtClassName()<< " "<< childStmt->getID(*sharedASTContext)<< "\n";
        }*/

        if(dyn_cast<BinaryOperator>(*child0)){
          BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(*child0);
      
          if (BinOpStmt->isAssignmentOp()) {
           
            const clang::Stmt *nodeSt=*child0;
            NodeExpression *nodeExp= new NodeExpression(nodeSt,&stackCondiotionaltBloc);
            //cout<<nodeExp->pyz3ApiFlatten()<<" FROM 479\n";
            //printConditionalStackContent(&stackCondiotionaltBloc);
            DefinitionContext* newDef= new DefinitionContext(nodeExp,&stackCondiotionaltBloc);
            
            Expr *lhs = (BinOpStmt->getLHS())->IgnoreImpCasts();
                        
            if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(lhs)) {
              if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                updateVariablesWithContextDefinition(newDef,VD);
              }
            }

            Expr *rhs= (BinOpStmt->getRHS())->IgnoreImpCasts();
            getVariablesUsedInExpression(rhs);

          }
        }
        

        if(isa<DeclStmt>(childStmt)){
          //cout<<"is<DeclStmt>\n";
          DeclStmt *decl=dyn_cast<DeclStmt>(childStmt);
          if((decl->isSingleDecl())){
            if (VarDecl *VD = dyn_cast<VarDecl>(decl->getSingleDecl())) {
            
              TrackedVariables var(VD->getNameAsString(), (VD->getType()).getAsString(),VD);
              pushIfNotInList(var);
              if(VD->hasInit ()){
                cout<< "NEEEED to update var with new def in array of defContext : --> size of defContext: "<< (var.vectorOfDefinitionContext).size()<<"\n";
              }

              cout<<repeatStr(tab,dept)<<"-"; var.print() ; 
              
              
            }

          }else{
              //decl->dump();
              for(auto declIt=decl->decl_begin(), endDeclIt=decl->decl_end(); declIt!=endDeclIt; declIt++){
                if (VarDecl *VD = dyn_cast<VarDecl>(*declIt)){

                  TrackedVariables var(VD->getNameAsString(), (VD->getType()).getAsString(),VD);   
                  pushIfNotInList(var);         
                  cout<<repeatStr(tab,dept)<<"-"; var.print(); cout<<" has init ? : "<< VD->hasInit ();
                  if(VD->hasInit()){
                    //Expr *initExpression= VD->getInit()
                    //need to get value
                  }else{
                    //set non-initialized into def context
                  }
            
                }else{
                  cout<<"NoT Cast variable\n";
                }
              }
          }
          //printConditionalStackContent(&stackCondiotionaltBloc);
          
        }
        exploreChildren(childStmt,dept++);
        
        /*if(dyn_cast<BinaryOperator>(childStmt)){
          BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(childStmt);
      
          if (BinOpStmt->isAssignmentOp()) {
              Expr *lhs = (BinOpStmt->getLHS())->IgnoreImpCasts();
              Expr *rhs = (BinOpStmt->getRHS())->IgnoreImpCasts();
              // def
              llvm::outs() << BinOpStmt->getOpcodeStr() << "\n";
              if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(lhs)) {
                if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                  
                 
                  
                }
              }
            }
        }*/

      }
      if(!stackParentBlocId->empty()){
        if(stackParentBlocId->top()== stmt->getID(*sharedASTContext)){
          //cout<<"current stmt"<< stmt->getID(*sharedASTContext)<< "| ";
          printStackContent(stackParentBlocId);
          //printConditionalStackContent(&stackCondiotionaltBloc);
          stackParentBlocId->pop();
          stackCondiotionaltBloc.pop();
          printStackContent(stackParentBlocId);
          //printConditionalStackContent(&stackCondiotionaltBloc);
        }
        
      }
      /*if(stackParentBlocId->top()==stmt->getID(*sharedASTContext)){

        
      }*/
}

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
private:
    ASTContext& astContext;
    string fileName;
    SourceManager &srcmgr;
    Rewriter &rewriter;
public:
    TargetProgramPoint targetPoint;
   

    MyASTVisitor(SourceManager& _srcmgr, Rewriter& _rewriter,ASTContext& Ctx )
      :srcmgr(_srcmgr), rewriter(_rewriter),astContext(Ctx)
    {
      cout << "MyASTVisitor Constructor:  \n"  << endl;
    }

    ~MyASTVisitor() {
      //cout << "MyASTVisitor Deconstructor: "  << endl;
    }

    bool VisitStmt(Stmt *st) {
   
    visitStmtCall++;
    
    //cout<<"-----!!!!!!! Size trackedVariables from Visitor call: "<<AllTrackedVariables.size()<< "\n";
    if(dyn_cast<BinaryOperator>(st)){
        BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(st);
        
        if (BinOpStmt->isAssignmentOp()) {
          Expr *lhs = (BinOpStmt->getLHS())->IgnoreImpCasts();
          Expr *rhs = (BinOpStmt->getRHS())->IgnoreImpCasts();
          //rhs->dump();
          const clang::Stmt *nodeSt=st;
          NodeExpression *nodeExp= new NodeExpression(nodeSt);
          //cout<<nodeExp->pyz3ApiFlatten()<<"\n";

        }
    }/*else if(dyn_cast<Dec>(st)){

    }*/
    
      return true;
    }
 

  bool VisitFunctionDecl(FunctionDecl *f) {
    ASTContext *astCtx=&astContext;
    string funcName = f->getNameInfo().getName().getAsString();
    //cout<<"visiting : "<< funcName<< " \n";
    if (funcName == functionNameDump) {
      Stmt *funcBody = f->getBody();
      // const std::unique_ptr<CFG> sourceCFG =CFG::buildCFG(f, funcBody,astCtx, CFG::BuildOptions());
      // //CM=  CFGStmtMap::Build	(	sourceCFG.get(),PM );	
      
      //sourceCFG->print(llvm::errs(), LangOptions(), true);

       cout << "\n\n //////////// \n Starting exploreChildren 0 \n\n //////////// \n"  << endl;
      exploreChildren(funcBody,0);
     
           
      return true;
    }

    return true;
  }
  
  };

class rightExpression;

class rightExpression{
public:
  Stmt *dataNodeElement;
  bool isConditional;
  rightExpression* leftElement;
  rightExpression* rightElement;
  string type;
  bool isBinaryNode;
  bool isUnaryNode;

  bool isLeafNode;
  string typeNode;
  
  bool isLiteralValue;
  bool isNull;
  bool isUndefined;
  rightExpression( Stmt *clangStmt){
      dataNodeElement=clangStmt;
      if(dyn_cast<BinaryOperator>(clangStmt)){
          isBinaryNode=true;
          isUnaryNode=false;       
          isLeafNode=false;

          const BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(clangStmt);
        
          Expr *lhs = (BinOpStmt->getLHS());
          clang::Stmt *leftStmt=dyn_cast<Stmt>(lhs);
          rightExpression *newNodeLeft= new rightExpression(leftStmt);
          leftElement= newNodeLeft;
          Expr *rhs = (BinOpStmt->getRHS());
          clang::Stmt *rightStmt=dyn_cast<Stmt>(rhs);
          rightExpression *newNoderight= new rightExpression(rightStmt);
          rightElement=newNoderight;

      }

    }

  
};


