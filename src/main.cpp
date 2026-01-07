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





#include "TargetProgramPoint.cpp"



TrackedVariables targetVariableUidSid;
std::vector<const Stmt *> AllVariablesStmt = {};
unsigned int sourceLineFromArg;
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
    return targetInstruction;

  /* // cout << "constraints : " << endl;
  for (pt::ptree::value_type &v : root.get_child("constraints")) {

    // cout << v.second.data() << endl;
  }
  return targetInstruction; */
  
}


#include "ConditionPathAstVisitor.cpp"

#include "ConditionPathAstConsumer.cpp"


//std::unique_ptr<ParentMap> PM ;//= llvm::make_unique<ParentMap>(FD->getBody());
//std::unique_ptr<CFGStmtMap> CM ;//= llvm::make_unique<CFGStmtMap>(cfg, PM.get());
//ParentMap *PM =nullptr;//= llvm::make_unique<ParentMap>(FD->getBody());
//CFGStmtMap *CM=nullptr ;//= llvm::make_unique<CFGStmtMap>(cfg, PM.get());



#include "MyASTVisitor.cpp" 

#include "pass1Visitor.cpp" 

#include "MyASTConsumer.cpp" 

#include "SsaAnalyzerAstVisitor.cpp" 

#include "SsaAstConsumer.cpp" 

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
    sourceLineFromArg = getTargetInstructionSourceLine(targetJsonFile);
    

  }else{
    cout<<"not enough argument\n";
    sourceLineFromArg = 22;
  }
 //cl::ParseCommandLineOptions(argc, argv, "My simple driver\n");


  if (!logFile) { // Shorthand for checking if open
        cerr << "File error!" << endl;
        return 1;
  }


    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
  CompilerInstance CI;
  DiagnosticOptions diagnosticOptions;
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
    
  SsaASTConsumer TheConsumer2(SourceMgr, ReWr, Ctx);
  SsaASTConsumer* consumer2Ssa=&TheConsumer2;
  AggregateASTConsumer astConsumers;
  // astConsumers.consumers.push_back(consumer0); //loop handling 

  //astConsumers.consumers.push_back(consumer3);
  astConsumers.consumers.push_back(consumer3);
  astConsumers.consumers.push_back(consumer2Ssa);




  //ParseAST(CI.getSema());
  ParseAST(CI.getPreprocessor(), &astConsumers, Ctx);
  
  //cout<<"running well list size " <<AllTrackedVariables.size()<<"\n";
  logFile<<"running well list size " <<AllTrackedVariables.size()<<"\n";
    

  
  //printListOfUniqueVariables();  

  //printAtomicVector(listOfAtomicElements);
 /* SmtSolver smt(listOfAtomicElements);
  smt.printListOfUniqueVariables();
  smt.generatePyZ3();
*/


// Inform Diagnostics that processing of a source file is beginning. 
  
  
  
  //ParseAST(CI.getPreprocessor(), &TheConsumer2, Ctx);

  /*
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, CI.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  const FileEntry *FileIn = FileMgr.getFile(argv[1]);
  SourceMgr.createMainFileID(FileIn);
  
  // Inform Diagnostics that processing of a source file is beginning. 
  TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());
  

  // Create an AST consumer instance which is going to get called by ParseAST.
  Rewriter& ReWr = TheRewriter;
  ConditionPathAstConsumer TheConsumer(SourceMgr, ReWr);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(TheCompInst.getPreprocessor(), &TheConsumer, TheCompInst.getASTContext());

    /*
  Py_SetProgramName(argv[0]); // optional but recommended
  Py_Initialize();
  PyRun_SimpleString("from time import time,ctime\n"
                      "print 'Today is',ctime(time())\n");
  Py_Finalize();*/

 
  return 0;
}
