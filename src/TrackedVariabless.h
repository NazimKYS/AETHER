#pragma once

class TrackedVariables {

public:
  std::string varName = "";
  std::string varType = "";
  const VarDecl* varRef = nullptr;  // ← critical!
  std::vector<Expr *> vectorOfDefExpressions = {};
  std::vector<Stmt *> vectorOfDefUseStmt = {};
  std::vector<DefinitionContext *> vectorOfDefinitionContext = {};
  void print();

  string printStr();
  TrackedVariables();
  TrackedVariables(string varname, string vartype, VarDecl *VD);
  TrackedVariables(const string varname, const string vartype,const VarDecl *VD) ;
  virtual string getClassName() ;
  void getCurrentDefinition();

  private:
  string getDefinitionFromVectorAt(uint index);

};
