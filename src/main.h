#ifndef MAIN_H_
#define MAIN_H_



#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/LangOptions.h"


#include "clang/Frontend/CompilerInstance.h"
//#include "clang/Frontend/HeaderSearchOptions.h"
//#include "clang/Frontend/PreprocessorOptions.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"

#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"


#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/CFGStmtMap.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
//#include "llvm/Support/Host.h" <-- obsolete
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/ErrorOr.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"


#include <sstream>
#include <string>
#include <iostream>
#include <typeinfo>
#include "clang/Driver/Options.h"

#include <iostream>
#include <stack>

/* #include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
//#include "json.hpp" <-- obsolete
#include <nlohmann/json.hpp> */

#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <utility>
#include <unordered_map>




#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"

#include <chrono>
#include <queue>

using namespace std;
using json = nlohmann::json;
//namespace pt = boost::property_tree;

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using clang::HeaderSearch;
using clang::HeaderSearchOptions;


using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");



llvm::StringRef get_source_text_raw(clang::SourceRange range,const clang::SourceManager &sm);
llvm::StringRef get_source_text(clang::SourceRange range,const clang::SourceManager &sm);
std::string binaryOpToStr(const clang::Stmt *StmtBinOp,const clang::SourceManager &sm,ASTContext *astContext);



class TrackedVariables;
class AtomicElementOfConditionPath;
class TargetProgramPoint;
class SmtSolver;
class NodeTool;
class NodeExpression;
struct SSAVariable; 
//class DefinitionContext;

ASTContext *sharedASTContext;
std::vector<TrackedVariables> AllTrackedVariables = {};
ofstream logFile("LogTracking.txt");
static int currentCallCounter=0;


void pushIfNotInList(TrackedVariables var);
void printListOfUniqueVariables();
//void updateVariablesWithContextDefinition(DefinitionContext *defCtx,VarDecl *VD);
void buildSemanticOfConditionPath(const Expr *expr);

void printAtomicVector(std::vector<AtomicElementOfConditionPath> listOfAtomicElements);
void trackVariables(Stmt *s) ;
void trackDefOfVariables(BinaryOperator *BinOpStmt) ;

  

void printVectorContent(vector<int64_t> v);

int indexOf(vector<int64_t> *v, int64_t element);

void extractUsedVariables(Expr *expr);
void extractUsedVariables(const Expr *expr);  

void extractVariablesFromConditiont(const Stmt *stmt,ASTContext *astContext);
unsigned int getTargetInstructionSourceLine(string targetJsonFile);
std::string rewriteWithSSA(const std::string& original, const std::vector<SSAVariable>& usedVars); 
/*
struct PathConditionElement {
    const clang::Expr* condition = nullptr;
    bool isTrueBranch = true;
};

struct RawPathCondition {
    bool found = false;
    const clang::Stmt* target = nullptr;
    std::vector<PathConditionElement> conditionStack;
    void dump() const;
};

// Function declaration
RawPathCondition collectPathToTarget(const clang::FunctionDecl* func, const clang::Stmt* target);
*/
#include "AtomicElementOfConditionPathV2.cpp"

#include "PathCondition.h"

//#include "SsaAnalyzerAstVisitor.cpp"


#endif


