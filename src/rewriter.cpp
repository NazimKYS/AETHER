class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

    // Now emit the rewritten buffer.
  //  TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs()); --> this will output to screen as what you got.
    std::error_code error_code;
    llvm::raw_fd_ostream outFile("output.txt", error_code, llvm::sys::fs::F_None);
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile); // --> this will write the result to outFile
    outFile.close();
  }
//as normal, make sure it matches your ASTConsumer constructor 
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {

    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<MyASTConsumer>(TheRewriter,&CI.getASTContext());
  }