#include "main.h"

class ConditionPathAstConsumer : public ASTConsumer {

private:
  ConditionPathAstVisitor visitor; // doesn't have to be private
  RawPathCondition rawPath;  // ← THIS IS REQUIRED   
  

  // Function to get the base name of the file provided by path
  string basename(std::string path) {
    cout << "std::string path " << path << "\n";
    return std::string(
        std::find_if(path.rbegin(), path.rend(), MatchPathSeparator()).base(),
        path.end());
  }

  // Used by std::find_if
  struct MatchPathSeparator {
    bool operator()(char ch) const { return ch == '/'; }
  };

public:
   ConditionPathAstConsumer(SourceManager& srcmgr, Rewriter& rewriter,ASTContext& Ctx ):
    visitor(srcmgr, rewriter,Ctx)
  {
  }

   
  
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Traverse the declaration using our AST visitor.
      visitor.TraverseDecl(*b);
      //visitor2->TraverseDecl(*b);
      
      //(*b)->dump();
    }
    //visitor2->astContext=sharedASTContext;
    return true;
  }

  virtual void HandleTranslationUnit(ASTContext &Context) {
    auto stratChronoAstBuild = high_resolution_clock::now();
    visitor.TraverseDecl(Context.getTranslationUnitDecl());
    auto endChronoAstBuild = high_resolution_clock::now();
    //visitor2->TraverseDecl(Context.getTranslationUnitDecl());
    duration<double, std::milli> msDoubleDurationAstBuild = endChronoAstBuild - stratChronoAstBuild;
    //std::cout <<" finding target statement in AST : " << msDoubleDurationAstBuild.count() << "ms  |  ";


    if((visitor.targetPoint).globalTargetStmt!=NULL){
      auto stratChronoFindParent = high_resolution_clock::now();
    (visitor.targetPoint).findParentStmt((visitor.targetPoint).globalTargetStmt, false);
    auto endChronoFindParent = high_resolution_clock::now();
    
    cout<<"\n\n *** Condition path *** \n\n";
    auto stratChronoGetConditionPath = high_resolution_clock::now();
    //(visitor.targetPoint).getConditionPath();
    (visitor.targetPoint).getConditionPathV2();
    cout<< "dumping ---------------------------------------- function \n";
    (visitor.targetPoint).exploreFunctionByStmt();
    auto endChronoGetConditionPath = high_resolution_clock::now();
 

    //Getting number of milliseconds as a double. 
    duration<double, std::milli> msDoubleDurationParentStmt = endChronoFindParent -  stratChronoFindParent ;
    duration<double, std::milli> msDoubleDurationConditionPath = endChronoGetConditionPath - stratChronoGetConditionPath;

    cout<<"\n\n ----- Durations stat ----- \n\n";
    std::cout <<" finding target statement in AST : " << msDoubleDurationAstBuild.count() << "ms  |  ";
    std::cout <<" Duration of computing parent nodes : " << msDoubleDurationParentStmt.count() << "ms  |  ";
    std::cout <<" Duration of computing condition path :" << msDoubleDurationConditionPath.count() << "ms\n";
    
    }else{
      std::cout <<" finding target statement in AST : " << msDoubleDurationAstBuild.count() << "ms\n";

      cout<<"Can't find target stmt check sourceline in target.json file\n";
    }
    int count=0;
    //StatementVisitor sv;
    
    (visitor.targetPoint).printPathConditions();
   // uncomment the following code up to std python solver
    cout << "Vraiables found :" << AllTrackedVariables.size()<<"\n";
     //<< std::endl; 
     //cout << "def found:"<<((AllTrackedVariables[0]).vectorOfDefExpressions).size()<< std::endl; printDefOfVariable(AllTrackedVariables[2]);

     //TranslationUnitDecl *decls =
     //this->resMgr.getCompilerInstance().getASTContext().getTranslationUnitDecl();
    // TranslationUnitDecl *decls
     

    //TargetOptions *TO = new TargetOptions();
    //TO->Triple = llvm::sys::getDefaultTargetTriple();
    //cout << (TO->Triple) << "\n";

    // std::system("python solver.py");
     cout<<" visitStmt called : " <<visitStmtCall<<"\n"; 

   

  }


};
/*virtual void HandleTranslationUnit(ASTContext &Ctx) {
    // Assume you already have 'targetStmt' (e.g., from source location)
  
    visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
    //if((visitor.targetPoint).globalTargetStmt!=NULL){
     
    
    for (const auto& decl : Ctx.getTranslationUnitDecl()->decls()) {
        if (const auto* func = dyn_cast<FunctionDecl>(decl)) {
            auto path = collectPathToTarget(func, (visitor.targetPoint).globalTargetStmt);//targetStmt);
            if (path.found) {
                // Store it!
                this->rawPath = std::move(path);
                path.dump(); // for debugging
                break;
            }
        }
    }
  }*/