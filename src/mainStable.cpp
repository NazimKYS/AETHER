#include "main.h"


//#include "AtomicElementOfConditionPathV2.cpp"


//#include <Python.h>



std::vector<AtomicElementOfConditionPath> listOfAtomicElements = {};
std::vector<uintptr_t> listOfVisitedNodes = {};


map<clang::Stmt*, vector<clang::Expr> >trackDefUseExpressions;
static unsigned long visitStmtCall=0;

void pushAddressIfNotInList(uintptr_t addrOfNode){
      
         auto it = find_if(
            listOfVisitedNodes.begin(), listOfVisitedNodes.end(),
            [addrOfNode](uintptr_t &obj) { return obj == addrOfNode; });
        if (it == listOfVisitedNodes.end()) {
          listOfVisitedNodes.push_back(addrOfNode);
          cout <<std::hex << addrOfNode <<" element pushed\n";
        }else{
          cout <<"Adress element Already exists\n";
        } //else { already exists }
}





void binaryOpToStrV2(const clang::Stmt *StmtBinOp,ASTContext *astContext){ //pass pointer of parent node
  NodeTool root(StmtBinOp);
  //cout<<"roooooooooooooot  \n"<<root.pyz3ApiFlatten()<<"\n";
  /*if(dyn_cast<BinaryOperator>(StmtBinOp)){
    //BinaryNode root(StmtBinOp);
    //cout<<"roooooooooooooot bin \n"<<root.pyz3ApiFlatten()<<"\n";


    NodeTool root2(StmtBinOp);
    cout<<"roooooooooooooot NodeTool \n"<<root2.pyz3ApiFlatten()<<"\n";

  }else{ 
    //UnaryNode root(StmtBinOp);
    //cout<<"roooooooooooooot un \n"<<root.pyz3ApiFlatten()<<"\n";
  }*/


    
}






llvm::StringRef get_source_text(clang::SourceRange range,
                                const clang::SourceManager &sm) {
  clang::LangOptions lo;

  // NOTE: sm.getSpellingLoc() used in case the range corresponds to a
  // macro/preprocessed source.
  auto start_loc = sm.getSpellingLoc(range.getBegin());
  auto last_token_loc = sm.getSpellingLoc(range.getEnd());
  auto end_loc = clang::Lexer::getLocForEndOfToken(last_token_loc, 0, sm, lo);
  auto printable_range = clang::SourceRange{start_loc, end_loc};
  
  return get_source_text_raw(printable_range, sm);
}

/**
 * Gets the portion of the code that corresponds to given SourceRange exactly
 * as the range is given.
 *
 * @warning The end location of the SourceRange returned by some Clang
 * functions (such as clang::Expr::getSourceRange) might actually point to the
 * first character (the "location") of the last token of the expression,
 * rather than the character past-the-end of the expression like
 * clang::Lexer::getSourceText expects. get_source_text_raw() does not take
 * this into account. Use get_source_text() instead if you want to get the
 * source text including the last token.
 *
 * @warning This function does not obtain the source of a macro/preprocessor
 * expansion. Use get_source_text() for that.
 */
llvm::StringRef get_source_text_raw(clang::SourceRange range,
                                    const clang::SourceManager &sm) {
  return clang::Lexer::getSourceText(
      clang::CharSourceRange::getCharRange(range), sm, clang::LangOptions());
}



std::string functionNameDump = "";


#include "BinaryElement.cpp"

#include "SmtSolver.cpp"

struct StmtAttributes {
  int64_t id;
  const Stmt *st;
  StmtAttributes(int64_t Id, const Stmt *St) {
    id = Id;
    st = St;
  }
};


//ASTContext *sharedASTContext;
Stmt *globalScopeTargetStmt;





unsigned getBitWidthForType(clang::QualType ty, const ExecutionEnv& env) {
    // Plan A: env empty → current machine, ASTContext already knows the sizes
    if (env.isEmpty()) {
        return (unsigned)sharedASTContext->getTypeSize(ty);
    }

    // Plan B: derive the C data model from env fields
    // Key question: is `long` 32-bit or 64-bit?
    //   LP64  (Linux/macOS 64-bit)   : long = 64
    //   LLP64 (Windows)              : long = 32  (long long = 64)
    //   ILP32 (any 32-bit target)    : long = 32
    bool longIs32 = false;

    // Rule 1: -m32 flag → force 32-bit mode (ILP32)
    for (const auto& flag : env.flags) {
        if (flag == "-m32") { longIs32 = true; break; }
    }

    // Rule 2: Windows OS → LLP64 model, long is 32 bits
    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };
    std::string osLow = toLower(env.os);
    if (osLow == "windows" || osLow == "win32" || osLow == "win64" || osLow == "win")
        longIs32 = true;

    // Rule 3: 32-bit arch → ILP32 model, long is 32 bits
    std::string archLow = toLower(env.arch);
    if (archLow == "i386" || archLow == "i686" || archLow == "x86" || archLow == "arm")
        longIs32 = true;

    // Rule 4: triple encodes both arch and OS (e.g. "i386-pc-linux-gnu", "x86_64-pc-windows-msvc")
    if (!env.triple.empty()) {
        std::string tripleLow = toLower(env.triple);
        if (tripleLow.find("i386")    != std::string::npos ||
            tripleLow.find("i686")    != std::string::npos ||
            tripleLow.find("windows") != std::string::npos)
            longIs32 = true;
    }

    // Map builtin type → bit width using the resolved model
    ty = ty.getCanonicalType();
    if (const auto* bt = dyn_cast<clang::BuiltinType>(ty.getTypePtr())) {
        switch (bt->getKind()) {
            case clang::BuiltinType::Char_S:
            case clang::BuiltinType::Char_U:
            case clang::BuiltinType::SChar:
            case clang::BuiltinType::UChar:
                return 8;
            case clang::BuiltinType::Short:
            case clang::BuiltinType::UShort:
                return 16;
            case clang::BuiltinType::Int:
            case clang::BuiltinType::UInt:
                return 32;
            case clang::BuiltinType::Long:
            case clang::BuiltinType::ULong:
                return longIs32 ? 32 : 64;
            case clang::BuiltinType::LongLong:
            case clang::BuiltinType::ULongLong:
                return 64;
            default:
                break;
        }
    }

    // Fallback: unknown type → ask ASTContext
    return (unsigned)sharedASTContext->getTypeSize(ty);
}

#include "TargetProgramPointV5.cpp"



TrackedVariables targetVariableUidSid;
std::vector<const Stmt *> AllVariablesStmt = {};
unsigned int sourceLineOfTargetStmt;
string targetJsonFile;


std::vector<int64_t> visitedStmt = {};



void printAtomicVector(std::vector<AtomicElementOfConditionPath> listOfAtomicElements){
  for (AtomicElementOfConditionPath element :listOfAtomicElements){
    if(element.typeElement=="var"){
      element.variable.print();
    }else if(element.typeElement=="binOpLogic"){
      cout <<" bin op logic : "<< std::string(element.BinOpStmt->getOpcodeStr()) <<"\n" ;

    }else if(element.typeElement=="binOpArith"){
      cout <<" bin op Arith : "<< std::string(element.BinOpStmt->getOpcodeStr()) <<"\n" ;

    }else if(element.typeElement=="lit"){
      cout <<" literal value  "<< element.literalValue <<"\n" ;

    }else if(element.typeElement=="unaryOp"){
      cout <<" unary Op : "<< element.unaryOperator <<"\n" ;

    }
  }
}



 
void printDefOfVariable(TrackedVariables v) {
  cout << "variable '" << v.varName
       << "' has : " << (v.vectorOfDefExpressions).size()
       << " def till target stmt\n";

  for (unsigned int i = 0; i < (v.vectorOfDefExpressions).size(); i++) {
    cout << "def " << i << " :\n";
    (v.vectorOfDefExpressions)[i]->dump();
  }
  cout << "\n";
}

void printVectorContent(vector<int64_t> v) {
  for (unsigned int i = 0; i < v.size(); i++) {
    cout << "vec elem : " << v[i] << " \n";
  }
}

int indexOf(vector<int64_t> *v, int64_t element) {
  /*if(element){
     cout<<"\n";
  }else{
      cout<<"******************empty***********\n";
  }*/
  auto it = find(v->begin(), v->end(), element);
  // If element was found
  if (it != v->end()) {
    int index = it - v->begin();
    return index;
  } else {
    // If the element is not
    // present in the vector
    return -1;
  }
}




unsigned int getTargetInstructionSourceLine(string targetJsonFile) {

    std::ifstream f(targetJsonFile);
    json data = json::parse(f);
    int targetInstruction = data["target"].get<int>();
    functionNameDump = data["dump"].get<std::string>();
    cout << "target instruction : " << targetInstruction << endl;
    cout << "function dump  : " << functionNameDump << endl;

    if (data.contains("executionEnv")) {
        auto& env = data["executionEnv"];
        sharedExecutionEnv.arch     = env.value("Arch",     "");
        sharedExecutionEnv.os       = env.value("OS",       "");
        sharedExecutionEnv.compiler = env.value("compiler", "");
        sharedExecutionEnv.triple   = env.value("triple",   "");
        if (env.contains("flags") && env["flags"].is_array()) {
            for (auto& flag : env["flags"])
                sharedExecutionEnv.flags.push_back(flag.get<std::string>());
        }
    }

    return targetInstruction;

  /* // cout << "constraints : " << endl;
  for (pt::ptree::value_type &v : root.get_child("constraints")) {

    // cout << v.second.data() << endl;
  }
  return targetInstruction; */
  
}


#include "ConditionPathAstVisitor.cpp"

#include "ConditionPathAstConsumer.cpp"






#include "MyASTVisitor.cpp" 

#include "pass1Visitor.cpp" 

#include "MyASTConsumer.cpp" 

/*#include "SsaAnalyzerAstVisitor.cpp" 

#include "SsaAstConsumer.cpp" */

#include "AggregateASTConsumer.cpp"



using namespace llvm;

//static cl::opt<std::string> FileName(cl::Positional, cl::desc("Input file"),cl::Required);

int main(int argc, const char **argv) {
  // std::cout << "Have " << argc << " arguments:" << std::endl;
  for (int i = 0; i < argc; ++i) {
    //   std::cout << argv[i] << std::endl;
  }
  int fakeargc = 2;
  int &fakeArg = fakeargc;
  if (argc >= 3) {
    string filePath = argv[2];
    targetJsonFile = filePath.substr(filePath.find_last_of("/\\") + 1);
    cout << "file name : " << targetJsonFile << "\n";
    sourceLineOfTargetStmt = getTargetInstructionSourceLine(targetJsonFile);
    

  }else{
    cout<<"not enough argument\n";
    sourceLineOfTargetStmt = 22;
  }
 //cl::ParseCommandLineOptions(argc, argv, "My simple driver\n");


  if (!logFile) { // Shorthand for checking if open
        cerr << "File error!" << endl;
        return 1;
  }


    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
  CompilerInstance CI;
  /*DiagnosticOptions diagnosticOptions;
  auto DiagClient = new clang::TextDiagnosticPrinter(llvm::errs(), &diagnosticOptions);
  CI.createDiagnostics(DiagClient, true);*/
  CI.createDiagnostics(NULL, false);
  
  using clang::TargetOptions;
  std::shared_ptr<TargetOptions> TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *PTI = TargetInfo::CreateTargetInfo(CI.getDiagnostics(), TO);
  CI.setTarget(PTI);
  CI.createFileManager();
  FileManager &FileMgr = CI.getFileManager();
  CI.createSourceManager(CI.getFileManager());
  SourceManager &SourceMgr =  CI.getSourceManager();

 

  CI.createPreprocessor(TU_Module);

  std::unique_ptr<ASTConsumer> astConsumer = CreateASTPrinter(NULL, "");
  CI.setASTConsumer(std::move(astConsumer));


  CI.createASTContext();
  CI.createSema(TU_Complete, NULL);

  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, CI.getLangOpts());


// Use getFileRef() → returns Expected<FileEntryRef>
auto fileRefOrErr = CI.getFileManager().getFileRef(argv[1]);
if (!fileRefOrErr) {
    std::cerr << "Error: could not open file " << argv[1] << std::endl;
    return 1;
}
clang::FileEntryRef fileRef = *fileRefOrErr;

// Now createFileID works
clang::FileID mainFileID = SourceMgr.createFileID(fileRef, clang::SourceLocation(), clang::SrcMgr::C_User);
CI.getSourceManager().setMainFileID(mainFileID);
// ----------------------------------------

  // SourceMgr.setMainFileID(CI.getSourceManager().createFileID(pFile, SourceLocation(), SrcMgr::C_User));
  //clang::FileID mainFileID = SourceMgr.createFileID(pFile.get(), SourceLocation(), SrcMgr::C_User); <-- obsolete

  // Inform Diagnostics that processing of a source file is beginning. 
  CI.getDiagnosticClient().BeginSourceFile(CI.getLangOpts(),&CI.getPreprocessor());
  
  Rewriter& ReWr = TheRewriter;
  std::error_code error_code;
  llvm::raw_fd_ostream outFile("output.txt", error_code, llvm::sys::fs::OF_None);
  ReWr.getEditBuffer(SourceMgr.getMainFileID()).write(outFile); // --> this will write the result to outFile
  outFile.close();


  // Create an AST consumer instance which is going to get called by ParseAST.
  ASTContext& Ctx =CI.getASTContext();
  ConditionPathAstConsumer TheConsumer3(SourceMgr, ReWr, Ctx);//new MySecASTConsumer(SourceMgr, ReWr, Ctx);
  ConditionPathAstConsumer* consumer3=&TheConsumer3;//new MySecASTConsumer(SourceMgr, ReWr, Ctx);
  


  MyASTConsumer TheConsumer(SourceMgr, ReWr, Ctx); 
  //std::unique_ptr<ASTConsumer> consumer1=  make_unique<MyASTConsumer>(SourceMgr, ReWr, Ctx);
  MyASTConsumer* consumer1=&TheConsumer; 

  //cout<<"pointer " << consumer1 <<"\n";
  //cout<<"ref     " << &TheConsumer <<"\n";

  Pass1Consumer TheConsumer0(SourceMgr, ReWr, Ctx);//new MySecASTConsumer(SourceMgr, ReWr, Ctx);
  Pass1Consumer* consumer0=&TheConsumer0;//new MySecASTConsumer(SourceMgr, ReWr, Ctx);
    
  /*SsaASTConsumer TheConsumer2(SourceMgr, ReWr, Ctx);
  SsaASTConsumer* consumer2Ssa=&TheConsumer2;*/
  AggregateASTConsumer astConsumers;
    // astConsumers.consumers.push_back(consumer0); //loop handling 
  

  //astConsumers.consumers.push_back(consumer3);
  astConsumers.consumers.push_back(consumer3);
 // astConsumers.consumers.push_back(consumer2Ssa);
  //astConsumers.consumers.push_back(consumer0);

 


  //ParseAST(CI.getSema());
  ParseAST(CI.getPreprocessor(), &astConsumers, Ctx);
  
  //cout<<"running well list size " <<AllTrackedVariables.size()<<"\n";
  logFile<<"running well list size " <<AllTrackedVariables.size()<<"\n";
   
 

 
  return 0;
}
