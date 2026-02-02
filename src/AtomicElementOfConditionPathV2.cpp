#pragma once
//#include "main.h"
//#include "TrackedVariables.cpp"

#include <stdint.h>

#include "DefinitionContext.h"


class Operand;
class TrackedVariables;

class Parenthesis;
class Negation;

class UnaryElement;
class AtomicElement;



class UnaryNode;
class BinaryNode;



//Node* mapNodeWithStmt(const Stmt *clangStmt);



//class VersionsOfVariable;


class AtomicElement {
public:AtomicElement(){}

virtual string getClassName() {
          return "Atomic element";
      }


};
class UnaryElement : public AtomicElement {

};
class Operand : public AtomicElement {
    const clang::Stmt  *operandAttribute;
public: Operand() {

    }
      virtual string getClassName() {
          return "Operand";
      }
};

class TrackedVariables {//: public Operand {
public:
  std::string varName = "";
  std::string varType = "";
  // auto varRef;
  const VarDecl* varRef = nullptr;  // ← critical!
  std::vector<Expr *> vectorOfDefExpressions = {};
  std::vector<Stmt *> vectorOfDefUseStmt = {};
  std::vector<DefinitionContext *> vectorOfDefinitionContext = {};
  void print(){
   std::cout << " varRef @ " << varRef << " : Variable name " << varName
              << "\t Variable type " << varType << "\n";
  }

  string printStr(){
    //uintptr_t adress=reinterpret_cast<uintptr_t > (varRef);
    string out= " varRef @   : Variable name " + varName+ "\t Variable type " + varType; 
    return out;
  }
  TrackedVariables(){};
  TrackedVariables(string varname, string vartype, VarDecl *VD) {
    varName = varname;
    varType = vartype;
    // varRef=varref;
    varRef = const_cast<VarDecl *>(VD);
    // std::cout << " varRef @ " << varRef << " : Variable name " << varName
      //        << "\t Variable type " << vartype << "\n";

    //print();
  }

  TrackedVariables(const string varname, const string vartype,const VarDecl *VD) {
    varName = varname;
    varType = vartype;
    // varRef=varref;
    varRef = VD;
     

    //print();
  }
  virtual string getClassName() {
          return "Variable";
  }



  void getCurrentDefinition(){
    //cout<< "getCurrentDefinition : this =="<<this->varName<<"\n";
    uint vecSize=vectorOfDefinitionContext.size();
    //cout<<"Call of getCurrentDefinition ... with vecSize " << vecSize <<"\n";
    logFile<<"Call of getCurrentDefinition ... with vecSize " << vecSize <<"\n";
    if(vecSize>0){

        if(!vectorOfDefinitionContext[vecSize-1]->isConditional){
            //cout<< this->varName<<"_"<< vectorOfDefinitionContext[vecSize-1]->getSsaLabel()<<" == " << vectorOfDefinitionContext[vecSize-1]->getDefinitionExpression() <<"\n";

        }else{
                
            //cout<< "getCurrentDefinition else conditional \n";
            
            //cout<< this->varName<<"_"<<vectorOfDefinitionContext[vecSize-1]->getSsaLabel()<<" == " << getDefinitionFromVectorAt(vecSize-1) <<"\n";
                
        }
    }
    
    
  }
  string  getLastSsaLable(){
    int SizeVecDefContext=vectorOfDefinitionContext.size();
    if(SizeVecDefContext>0){
        string retunedValue="_"+to_string((this->vectorOfDefinitionContext[SizeVecDefContext-1])->getSsaLabel());
        //cout<< "call of getLAstSSAlabel: "<<(this->vectorOfDefinitionContext[this->vectorOfDefinitionContext.size()-1])->getSsaLabel()<< "which means :"<<retunedValue;
        
        return retunedValue;//"_"+(this->vectorOfDefinitionContext[this->vectorOfDefinitionContext.size()-1])->getSsaLabel();
    }else if(vectorOfDefinitionContext.size()==0){
        //cout<<this->varName <<" has no definition yet !"<<"\n";
        return "_Null";
    }
    else{
        return "Undefined";
    }
  }
    
private:
  string getDefinitionFromVectorAt(int index){
    currentCallCounter++;
    logFile<<" starting getDefinitionFromVectorAt CALL COUNTER :# "<< currentCallCounter<<"\n";
    string def="";
    //cout<<"call getDefinitionFromVectorAt index "<<index <<"\n";

    if(index>=0){
        if(this->vectorOfDefinitionContext[index]->isConditional){
            string cond=vectorOfDefinitionContext[index]->getConditionalExpression();
            string arg1=vectorOfDefinitionContext[index]->getDefinitionExpression();
            string arg2="Null";
            if(index-1>=0){
                arg2=getDefinitionFromVectorAt(index -1);
            }else{
                logFile<<"@@@@@@@@ Unhandled USE case to complete log \n";
            }
            
            
            def = "ite ("+ cond + " ( "+ arg1 + ") ("+ arg2 +") )";
            return def;
        }else{
            def=vectorOfDefinitionContext[index]->getDefinitionExpression();
            return def;

        }

    }else{
        return "INDEX ERROR < 0";
    }
  }
  /*
  void printvarWithDef(){

    string AllDefinitions="";
    
    //for(unsigned int i=0;i<vectorOfDefinitionContext.size();i++){
    for(DefinitionContext *def : vectorOfDefinitionContext){
        AllDefinitions=AllDefinitions+"\t"+def->getDefinitionExpression()+ "\n";
    }
   std::cout << " varRef @ " << varRef << " : Variable name " << varName
              << "\t Variable type " << varType << ": ["+AllDefinitions+"]\n";
  }
  */
};

void pushIfNotInList(TrackedVariables var){
      const VarDecl *VD =var.varRef;
         auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it == AllTrackedVariables.end()) {
          AllTrackedVariables.push_back(var);
        } else { 
            //cout<<" &&&&& already exists &&& : " <<var.varName << "\n"; 
            }
}
void printListOfUniqueVariables(){
        for (TrackedVariables v : AllTrackedVariables){
          v.print();
        }
}

string getCurrentSsaLabelOfVariable(const VarDecl *VD){
    /*currentCallCounter++;
    logFile<<" starting getCurrentSsaLabelOfVariable CALL COUNTER :# "<< currentCallCounter<<"\n";*/
    auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it != AllTrackedVariables.end()) {

          return it->getLastSsaLable(); //need to check size defContext if >0 there a def so there is ssaLable but if size ==0 it means Null or not yet filled with def context beacause of the 1st pass related to backward AST traversing in condPathComputig 
        }else{
                //end without finding the var in tracked var
                //cout << "Definitions are not Not yet tracked since the list has no instance of this var maybe coming from computeCondPath\n";
            return "";//Definitions are not Not yet tracked since the list has no instance of this var maybe coming from computeCondPath\n";
            /*cout<<"\n AllTrackedVariables  : start checking size vec"<<  AllTrackedVariables.size()<< "\n";
            for (TrackedVariables v : AllTrackedVariables){
                v.print();
            }
            cout<<"\n AllTrackedVariables  : end checking \n";*/
            
        }
        //else not found

}


void updateVariablesWithContextDefinition(DefinitionContext *defCtx,VarDecl *VD){

    auto it = find_if(
            AllTrackedVariables.begin(), AllTrackedVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it != AllTrackedVariables.end()) {
          (it->vectorOfDefinitionContext).push_back(defCtx);
        }//else not found
            
         
    
}











std::ostream& operator<<(std::ostream& strm, const TrackedVariables& var) {
    return strm << " " << var.varName << " ";
}


/*
class AtomicElementOfConditionPath{
  
public:
string typeElement; // var , lit, binOpArith, unaryOp , binOpLogic
  TrackedVariables variable;
  const BinaryOperator *BinOpStmt;
  string literalValue;
  string typeOfLiteral;
  string unaryOperator;
  AtomicElementOfConditionPath(string typeelement,TrackedVariables variabl){
      typeElement=typeelement;
      variable=variabl;
      //BinaryOperator=nullptr;
      //literalValue= NULL;
  }
   AtomicElementOfConditionPath(string typeelement,const BinaryOperator *binopstmt){
      typeElement=typeelement;
      BinOpStmt=binopstmt;
      //variable= NULL;
     //literalValue= NULL;


  }

 
  AtomicElementOfConditionPath(string typeelement,string literalvalue,string literalType){
      typeElement=typeelement;
      literalValue=literalvalue;
      typeOfLiteral = literalType;
      cout << "letteral value created "<< literalvalue <<"\n";
      //BinOpStmt=nullptr;
      //variable NULL;
  }

  AtomicElementOfConditionPath(string typeelement,string unaryoperator){
      typeElement=typeelement;
      unaryOperator=unaryoperator;
    
      //BinOpStmt=nullptr;
      //variable NULL;
  }

};
*/

class AtomicElementOfConditionPath {
public:
    std::string typeElement = "";
    TrackedVariables variable{};              // value-initialize (calls default constructor)
    const BinaryOperator* BinOpStmt = nullptr;
    std::string literalValue = "";
    std::string typeOfLiteral = "";
    std::string unaryOperator = "";

    // Constructor for variable
    AtomicElementOfConditionPath(const std::string& typeelement, const TrackedVariables& variabl)
        : typeElement(typeelement), variable(variabl) {}

    // Constructor for binary op
    AtomicElementOfConditionPath(const std::string& typeelement, const BinaryOperator* binopstmt)
        : typeElement(typeelement), BinOpStmt(binopstmt) {}

    // Constructor for literal
    AtomicElementOfConditionPath(const std::string& typeelement, const std::string& literalvalue, const std::string& literalType)
        : typeElement(typeelement), literalValue(literalvalue), typeOfLiteral(literalType) {
        std::cout << "literal value created " << literalvalue << "\n";
    }

    // Constructor for unary op
    AtomicElementOfConditionPath(const std::string& typeelement, const std::string& unaryoperator)
        : typeElement(typeelement), unaryOperator(unaryoperator) {}
};

static int pyz3ApiCallCounter=0;



class NodeTool{
   
public:
    const Stmt *dataNodeElement = nullptr;
    NodeTool *leftNode = nullptr;
    NodeTool *rightNode = nullptr; // set this node if unary
    
      
    bool isBinaryNode;
    bool isUnaryNode;
    bool isFakeNode;
    bool isLeafNode;
    string typeNode;
    string typeLeafNode;


    NodeTool(string typenode,NodeTool *node){ //default constructor
        isFakeNode=true;
        //dataNodeElement=nullptr;
        typeNode=typenode;
        rightNode=node;

    }
    NodeTool(const Stmt *clangStmt){
        //cout<<"NodeTool cnstructor\n";
        //clangStmt->dump();
        dataNodeElement=clangStmt;
        if(dyn_cast<BinaryOperator>(clangStmt)){
            isBinaryNode=true;
            isUnaryNode=false;
            isFakeNode=false;
            isLeafNode=false;

            const BinaryOperator *BinOpStmt=dyn_cast<BinaryOperator>(clangStmt);
            //cout<<"New node "<<clangStmt->getStmtClassName() << " "<< std::string(BinOpStmt->getOpcodeStr())<<"\n";
          
            Expr *lhs = (BinOpStmt->getLHS());
            const clang::Stmt *leftStmt=dyn_cast<Stmt>(lhs);
            NodeTool *newNodeLeft= new NodeTool(leftStmt);
            leftNode= newNodeLeft;
            Expr *rhs = (BinOpStmt->getRHS());
            const clang::Stmt *rightStmt=dyn_cast<Stmt>(rhs);
            NodeTool *newNoderight= new NodeTool(rightStmt);
            rightNode=newNoderight;

        }else{
            //leftNode=nullptr;  

            if (dyn_cast<ParenExpr>(clangStmt)){
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                const ParenExpr *parenExpr= dyn_cast<ParenExpr>(clangStmt);
                
                const clang::Stmt *subStmt= dyn_cast<Stmt> (parenExpr->getSubExpr());
                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                NodeTool *newNoderight= new NodeTool(subStmt);
                rightNode=newNoderight;
            }else if(isa<UnaryOperator>(clangStmt)){ //logical not 
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                const UnaryOperator *unaryOp=dyn_cast<UnaryOperator>(clangStmt);
                const clang::Stmt *subStmt= dyn_cast<Stmt> (unaryOp->getSubExpr());        
                NodeTool *newNoderight= new NodeTool(subStmt);
                rightNode=newNoderight;

                
                
            }else if(isa<ImplicitCastExpr>(clangStmt)){ //implCast
                isBinaryNode=false;
                isUnaryNode=true;
                isFakeNode=false;
                isLeafNode=false;

                //cout<<"New node "<<clangStmt->getStmtClassName() << "\n";
                const Expr *impCastExpr=(dyn_cast<CastExpr>(clangStmt))->getSubExpr();
                const clang::Stmt *subStmt= dyn_cast<Stmt> (impCastExpr);
                NodeTool *newNoderight= new NodeTool(subStmt);
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
                    //is there any constructor in TrackedVariables class such that the var has not a definition ?
                    typeLeafNode=  vd->getNameAsString()+getCurrentSsaLabelOfVariable(vd);  
                    //cout<< "CHECKING IF ********************** "<<typeLeafNode <<"  should be : "<< vd->getNameAsString()<<getCurrentSsaLabelOfVariable(vd);
                    //dre->dump();
                    pushIfNotInList(leftVar);                              
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
                
            }//missing to handla other class of leafNodes
        }

    }
  

    string pyz3ApiFlatten(){
        pyz3ApiCallCounter++;
        
        string outputStr="";
        if(dataNodeElement){
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

                if(std::string(BinOpStmt->getOpcodeStr())=="&&" || std::string(BinOpStmt->getOpcodeStr())=="||"){  

                    if(std::string(BinOpStmt->getOpcodeStr())=="&&"){
                        opCodeStr="And";
                    }else{
                        opCodeStr="Or";
                    }
                    outputStr = opCodeStr+"(\n\t"+leftStr +" ,"+rightStr+"\n )";
                    
                    
                }else{
                    opCodeStr=std::string(BinOpStmt->getOpcodeStr());
                    outputStr = "( "+leftStr +" "+ opCodeStr+" "+ rightStr+" )";
                }



            }else if(isUnaryNode){
                //cout<< "z3 call counter una:  "<<pyz3ApiCallCounter<<"\n";
                if (dyn_cast<ParenExpr>(dataNodeElement)){
                    outputStr="( "+ rightNode->pyz3ApiFlatten() + " )\n";
                }else if(isa<UnaryOperator>(dataNodeElement)){  //check negat
                    const UnaryOperator *unaryOp=dyn_cast<UnaryOperator>(dataNodeElement);
                    if(std::string(unaryOp->getOpcodeStr(unaryOp->getOpcode()))=="!"){
                        
                        outputStr="Not( "+ rightNode->pyz3ApiFlatten() + " )\n";
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

    
    ~NodeTool() {
        delete leftNode;
        delete rightNode;
    }

    // Prevent copies (optional but safe)
    NodeTool(const NodeTool&) = delete;
    NodeTool& operator=(const NodeTool&) = delete;


};
