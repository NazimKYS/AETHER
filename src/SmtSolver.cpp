#pragma once
#include "main.h"


class SmtSolver{
public:
    vector<TrackedVariables> listOfUniqueVariables;
    std::vector<AtomicElementOfConditionPath> listOfAtomicElements;
    std::vector<string> SmtConditionPathExpressions={};
    SmtSolver(std::vector<AtomicElementOfConditionPath> listofatomicelements){
      listOfAtomicElements=listofatomicelements;
      for(AtomicElementOfConditionPath atomicElem : listofatomicelements){
        if(atomicElem.typeElement=="var"){
          pushIfNotInList(atomicElem.variable);
        }
      }
    }
    void pushIfNotInList(TrackedVariables var){
      const VarDecl *VD =var.varRef;
         auto it = find_if(
            listOfUniqueVariables.begin(), listOfUniqueVariables.end(),
            [VD](const TrackedVariables &obj) { return obj.varRef == VD; });
        if (it == listOfUniqueVariables.end()) {
          listOfUniqueVariables.push_back(var);
        } //else { already exists }
    }
    void printListOfUniqueVariables(){
        for (TrackedVariables v : listOfUniqueVariables){
          v.print();
        }
    }

    string getLogicalBinaryOperator(string binOp){
      if(binOp=="&&"){
          return "And";
      }else if(binOp=="||"){
          return "Or";
      }else if(binOp==">"){
          return ">";
      }else if(binOp==">="){
          return ">=";
      }else if(binOp=="<"){
          return "<";
      }else if(binOp=="<="){
          return "<=";
      }else if(binOp=="=="){
          return "==";
      }else if(binOp=="!="){
          return "!=";
      }else{
          return "not handled yet";

      }
    }
    
    void generateSmtExpressions(){
      string rightExpression="";
      string leftExpression="";
      string middleOp="";
      string conditionState="";
      
      string subExpression="";
      vector<BinaryElement> listComplexStmt={};
      stack<BinaryElement*> stackBinOp;
      int logicalBinOpCounter=0;

      
      // typeElement var , lit, binOp, unaryOp


       for (unsigned int i = 0; i < listOfAtomicElements.size()-2; ++i) {
          AtomicElementOfConditionPath atomicElem=listOfAtomicElements[i];

          if(atomicElem.typeElement=="unaryOp"){
            if(atomicElem.unaryOperator=="Not"){
              conditionState="Not";

            }else{
              conditionState="";
              //rightExpression=rightExpression+" )"; //check if left not empty

            }
          } else if(atomicElem.typeElement=="binOpLogic"){

            //cout<<"comlex sub expression SMT\n";
            BinaryElement *currentElement=new BinaryElement(std::string(atomicElem.BinOpStmt->getOpcodeStr()),conditionState);
            if(stackBinOp.empty()){
                stackBinOp.push(currentElement);
            }else{ //assume top stack element has empty element 
                (stackBinOp.top())->addElement(currentElement);
                stackBinOp.push(currentElement);
            }
            
            //if(listOfAtomicElements[i+1].typeElement=="binOp")
              
                //conditionState=conditionState+ " "+getLogicalBinaryOperator(std::string(atomicElem.BinOpStmt->getOpcodeStr()))+" (";
                 
            }else if(atomicElem.typeElement=="binOpArith"){
                // we have < > == ...

                middleOp=" "+getLogicalBinaryOperator(std::string(atomicElem.BinOpStmt->getOpcodeStr())) + " ";
                //right expression check if var or lit
                if(listOfAtomicElements[i+1].typeElement=="var"){
                  rightExpression =listOfAtomicElements[i+1].variable.varName;
                }else if(listOfAtomicElements[i+1].typeElement=="lit"){
                  rightExpression=listOfAtomicElements[i+1].literalValue;
                }

                //left expression check if var or lit
                if(listOfAtomicElements[i+2].typeElement=="var"){
                  leftExpression =listOfAtomicElements[i+2].variable.varName;
                }else if(listOfAtomicElements[i+2].typeElement=="lit"){
                  leftExpression=listOfAtomicElements[i+2].literalValue;
                }

                //right and left and middl are ready conditionState+
                subExpression =   " ( "+ rightExpression +" "+
                middleOp+ " " + leftExpression + " )";
                if(stackBinOp.empty()){  
                  subExpression=conditionState+subExpression;
                  SmtConditionPathExpressions.push_back(subExpression);
                }else{

                    if((stackBinOp.top())->hasEmptyElement()){
                      (stackBinOp.top())->addElement(subExpression);

                      if((stackBinOp.top())->hasEmptyElement()==false){
                          if(stackBinOp.size()==1){
                              SmtConditionPathExpressions.push_back((stackBinOp.top())->getFullExpression());
                          }
                          stackBinOp.pop();
                      }

                      //smtExpressions.push_back((stackBinOp.top())->getFullExpression());
                      //stackBinOp.pop();
                    }else{
                        cout<<" top stack element is full stack not empty \n";
                        bool addedSimpleExpresionToStackElement=false;
                        //top stack element is full
                        for(int k=stackBinOp.size(); k>1 ;k--){
                            if((stackBinOp.top())->hasEmptyElement()==false){

                                stackBinOp.pop();
                            }else{
                                (stackBinOp.top())->addElement(subExpression);
                                addedSimpleExpresionToStackElement=true;
                                break;
                            }
                        }

                        
                        if(stackBinOp.size()==1){
                                SmtConditionPathExpressions.push_back((stackBinOp.top())->getFullExpression());
                            }
                        stackBinOp.pop();
                        if(!addedSimpleExpresionToStackElement){
                            SmtConditionPathExpressions.push_back(subExpression);
                        }
                        
                    }
                  
                  //listComplexStmt.push_back(subExpression);


                }





            }
        }
        /*for(string exp:SmtConditionPathExpressions){
          cout<< exp <<"\n";
        }*/
    }
    void generatePyZ3(){
      cout<< "\n\n\n******PyZ3 script generation *************\n\n\n";
      string header="from z3 import *";
      string varDeclarations="";
      string conditionPath="";
      string checkModels="";
      string fullFile="";
      generateSmtExpressions();
      // missing all models 

      for (TrackedVariables v : listOfUniqueVariables){
          if(v.varType=="int" || v.varType=="long"){
            varDeclarations=varDeclarations + v.varName + " = Int('"+ v.varName+"') \n";
          }
        }

      //condition path initialisation
      conditionPath="s = Solver()\n";
      conditionPath=conditionPath+"s.add( ";
      for(unsigned int i = 0; i < SmtConditionPathExpressions.size()-1; ++i)  {
        string expr=SmtConditionPathExpressions[i];
        conditionPath=conditionPath+ expr +", ";

      }
      conditionPath=conditionPath+SmtConditionPathExpressions[SmtConditionPathExpressions.size()-1]+ " )" ;
      checkModels ="print(s.check()) \n";
      checkModels=checkModels+"print(s.model()) \n";

      fullFile=header+ "\n\n"+ varDeclarations + "\n\n" + conditionPath +"\n\n"+checkModels;

    
      cout<<fullFile;

     }




};
