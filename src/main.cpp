#include "main.h"

static unsigned long visitStmtCall = 0;

llvm::StringRef get_source_text(clang::SourceRange range, const clang::SourceManager &sm) {
    clang::LangOptions lo;
    auto start_loc      = sm.getSpellingLoc(range.getBegin());
    auto last_token_loc = sm.getSpellingLoc(range.getEnd());
    auto end_loc        = clang::Lexer::getLocForEndOfToken(last_token_loc, 0, sm, lo);
    return get_source_text_raw(clang::SourceRange{start_loc, end_loc}, sm);
}

llvm::StringRef get_source_text_raw(clang::SourceRange range, const clang::SourceManager &sm) {
    return clang::Lexer::getSourceText(
        clang::CharSourceRange::getCharRange(range), sm, clang::LangOptions());
}

std::string functionNameDump = "";

#include "BinaryElement.cpp"
#include "SmtSolver.cpp"

struct StmtAttributes {
    int64_t    id;
    const Stmt *st;
    StmtAttributes(int64_t Id, const Stmt *St) : id(Id), st(St) {}
};

Stmt *globalScopeTargetStmt;

unsigned getBitWidthForType(clang::QualType ty, const ExecutionEnv& env) {
    if (env.isEmpty())
        return (unsigned)sharedASTContext->getTypeSize(ty);

    bool longIs32 = false;
    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    for (const auto& flag : env.flags)
        if (flag == "-m32") { longIs32 = true; break; }

    std::string osLow = toLower(env.os);
    if (osLow == "windows" || osLow == "win32" || osLow == "win64" || osLow == "win")
        longIs32 = true;

    std::string archLow = toLower(env.arch);
    if (archLow == "i386" || archLow == "i686" || archLow == "x86" || archLow == "arm")
        longIs32 = true;

    if (!env.triple.empty()) {
        std::string tripleLow = toLower(env.triple);
        if (tripleLow.find("i386")    != std::string::npos ||
            tripleLow.find("i686")    != std::string::npos ||
            tripleLow.find("windows") != std::string::npos)
            longIs32 = true;
    }

    ty = ty.getCanonicalType();
    if (const auto* bt = dyn_cast<clang::BuiltinType>(ty.getTypePtr())) {
        switch (bt->getKind()) {
            case clang::BuiltinType::Char_S:  case clang::BuiltinType::Char_U:
            case clang::BuiltinType::SChar:   case clang::BuiltinType::UChar:    return 8;
            case clang::BuiltinType::Short:   case clang::BuiltinType::UShort:   return 16;
            case clang::BuiltinType::Int:     case clang::BuiltinType::UInt:     return 32;
            case clang::BuiltinType::Long:    case clang::BuiltinType::ULong:    return longIs32 ? 32 : 64;
            case clang::BuiltinType::LongLong: case clang::BuiltinType::ULongLong: return 64;
            default: break;
        }
    }
    return (unsigned)sharedASTContext->getTypeSize(ty);
}

#include "TargetProgramPointV5.cpp"

unsigned int sourceLineOfTargetStmt;
string       targetJsonFile;

unsigned int getTargetInstructionSourceLine(string targetJsonFile) {
    std::ifstream f(targetJsonFile);
    json data = json::parse(f);

    int targetInstruction = data["target"].get<int>();
    functionNameDump      = data["dump"].get<std::string>();
    cout << "target instruction : " << targetInstruction << "\n";
    cout << "function dump      : " << functionNameDump  << "\n";

    if (data.contains("executionEnv")) {
        auto& env = data["executionEnv"];
        sharedExecutionEnv.arch     = env.value("Arch",     "");
        sharedExecutionEnv.os       = env.value("OS",       "");
        sharedExecutionEnv.compiler = env.value("compiler", "");
        sharedExecutionEnv.triple   = env.value("triple",   "");
        if (env.contains("flags") && env["flags"].is_array())
            for (auto& flag : env["flags"])
                sharedExecutionEnv.flags.push_back(flag.get<std::string>());
    }

    if (data.contains("constraints") && data["constraints"].is_array()) {
        for (auto& c : data["constraints"]) {
            UserConstraint uc;
            uc.variable = c.value("variable", "");
            uc.op       = c.value("operator", "");
            uc.value    = c["value"].is_number() ? c["value"].dump() : c.value("value", "");
            if (!uc.variable.empty() && !uc.op.empty())
                sharedConstraints.push_back(uc);
        }
    }

    return targetInstruction;
}

#include "MyASTVisitor.cpp"
#include "ConditionPathAstVisitor.cpp"
#include "ConditionPathAstConsumer.cpp"
#include "AggregateASTConsumer.cpp"

using namespace llvm;

int main(int argc, const char **argv) {
    if (argc >= 3) {
        string filePath = argv[2];
        targetJsonFile = filePath.substr(filePath.find_last_of("/\\") + 1);
        cout << "file name : " << targetJsonFile << "\n";
        sourceLineOfTargetStmt = getTargetInstructionSourceLine(targetJsonFile);
    } else {
        cout << "not enough arguments\n";
        sourceLineOfTargetStmt = 22;
    }

    CompilerInstance CI;
    CI.createDiagnostics(NULL, false);

    std::shared_ptr<clang::TargetOptions> TO = std::make_shared<clang::TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *PTI = TargetInfo::CreateTargetInfo(CI.getDiagnostics(), TO);
    CI.setTarget(PTI);

    CI.createFileManager();
    CI.createSourceManager(CI.getFileManager());
    SourceManager &SourceMgr = CI.getSourceManager();

    // Configure header search so system headers (e.g. stdint.h) can be found.
    // ResourceDir provides Clang's built-in headers (stdint.h, limits.h, …).
    // /usr/include covers libc/system headers.
    {
        HeaderSearchOptions &HSO = CI.getHeaderSearchOpts();
        HSO.ResourceDir = clang::CompilerInvocation::GetResourcesPath(
            argv[0], (void *)(intptr_t)main);
        HSO.AddPath("/usr/local/include",              clang::frontend::System, false, false);
        HSO.AddPath("/usr/include/x86_64-linux-gnu",  clang::frontend::System, false, false);
        HSO.AddPath("/usr/include",                    clang::frontend::System, false, false);
    }

    CI.createPreprocessor(TU_Module);
    CI.createASTContext();
    CI.createSema(TU_Complete, NULL);

    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, CI.getLangOpts());

    auto fileRefOrErr = CI.getFileManager().getFileRef(argv[1]);
    if (!fileRefOrErr) {
        std::cerr << "Error: could not open file " << argv[1] << std::endl;
        return 1;
    }
    clang::FileID mainFileID = SourceMgr.createFileID(
        *fileRefOrErr, clang::SourceLocation(), clang::SrcMgr::C_User);
    SourceMgr.setMainFileID(mainFileID);
    CI.getDiagnosticClient().BeginSourceFile(CI.getLangOpts(), &CI.getPreprocessor());

    ASTContext& Ctx = CI.getASTContext();
    ConditionPathAstConsumer theConsumer(SourceMgr, TheRewriter, Ctx);
    AggregateASTConsumer astConsumers;
    astConsumers.consumers.push_back(&theConsumer);

    ParseAST(CI.getPreprocessor(), &astConsumers, Ctx);

    return 0;
}
