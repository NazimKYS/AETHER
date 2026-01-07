#pragma once
class DefinitionContext{

public:
  bool isConditional;
  bool hasInitialValue;
  bool isPhiNode;
  int ssaLabel;

  NodeExpression *fullDefinition;
  
  vector<NodeTool*> listOfConditions;
  DefinitionContext(NodeExpression *def, stack<NodeTool*> *myStack);

  
  string getDefinitionExpression();
  string getConditionalExpression();
  int getSsaLabel();
  void setSsaLabel(int index);

};
