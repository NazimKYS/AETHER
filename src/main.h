#ifndef MAIN_H_
#define MAIN_H_

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/LangOptions.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/Utils.h"

#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Preprocessor.h"

#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/raw_ostream.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <stack>
#include <map>
#include <unordered_map>
#include <chrono>
#include <set>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

using namespace clang;
using clang::HeaderSearch;
using clang::HeaderSearchOptions;

using std::chrono::high_resolution_clock;
using std::chrono::duration;

class TargetProgramPoint;
struct SSAVariable;

ASTContext *sharedASTContext;

struct ExecutionEnv {
    std::string arch;
    std::string os;
    std::string compiler;
    std::vector<std::string> flags;
    std::string triple;

    bool isEmpty() const {
        return arch.empty() && os.empty() && compiler.empty() && triple.empty() && flags.empty();
    }
};

ExecutionEnv sharedExecutionEnv;

struct UserConstraint {
    std::string variable;
    std::string op;
    std::string value;
};

std::vector<UserConstraint> sharedConstraints;

unsigned getBitWidthForType(clang::QualType ty, const ExecutionEnv& env);

#endif
