#pragma once
#include "main.h"



class BinaryElement{
public:
  string op;
  string negationState;
  string strElement1;
  string strElement2;
  BinaryElement* element1;
  BinaryElement* element2;
  public: BinaryElement(){
    strElement1="";
    strElement2="";
    element1=NULL;
    element2=NULL;
  }
  public:BinaryElement(string opt, string conditionState){
    if(opt=="&&") {
        op ="And";
    }else if(opt=="||"){
        op="Or";
    }else{
        cout<<"Logical operator not handled from binary element \n";
    }
    //op =  opt;
    negationState=conditionState;
    strElement1="";
    strElement2="";
    element1=NULL;
    element2=NULL;
  }
  void addElement(string stmt){
    if(strElement1==""&& element1==NULL ){
      strElement1=stmt;
    }else if(strElement2=="" && element2==NULL)
      strElement2=stmt;
  }
  void addElement( BinaryElement* element){
    if(strElement1==""&& element1==NULL  ){
      element1 = element;
    }else if(strElement2=="" && element2==NULL)
      element2 = element;
  }
  bool hasEmptyElement(){
    return ( (element1==NULL && strElement1=="") || (element2==NULL && strElement2=="") );
  }
  string getFullExpression(){
    string expression="";
    if(strElement1 !="" && strElement2 !=""){
        expression=op+"( "+strElement1+" , " +strElement2+" )" ;
    }else if(element1!= NULL && element2!= NULL){
        expression=op+"( "+element1->getFullExpression()+" , " +element2->getFullExpression()+" )" ;
    }else if(element1!= NULL && strElement1 !=""){
        expression=op+"( "+element1->getFullExpression()+" , " +strElement1+" )" ;
    }else if(element1!= NULL && strElement2 !=""){
        expression=op+"( "+element1->getFullExpression()+" , " +strElement2+" )" ;
    }else if(strElement1 !="" && element2!= NULL){
        expression=op+"( "+strElement1+" , " +element2->getFullExpression()+" )" ;
    }else if(strElement2 !="" && element2!= NULL){
        expression=op+"( "+strElement2+" , " +element2->getFullExpression()+" )" ;
    }else{
        expression="empty";
    }   

    //cout<< expression<<"\n";
    if(negationState!=""){
      expression= "Not("+expression+")"; 
    }
    return expression;
  }

};
