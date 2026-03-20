#include "main.h"

ASTContext *sharedASTContext = nullptr;
ExecutionEnv sharedExecutionEnv;
std::vector<UserConstraint> sharedConstraints;
KnowledgeBase sharedKnowledgeBase;

// ── Knowledge-base loading ────────────────────────────────────────────────────

void loadKnowledgeBase(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[KB] warning: could not open " << path
                  << " — falling back to built-in defaults\n";
        return;
    }

    // allow_exceptions=false: returns a discarded value instead of throwing.
    json kb = json::parse(f, nullptr, /*allow_exceptions=*/false);
    if (kb.is_discarded()) {
        std::cerr << "[KB] parse error: invalid JSON in " << path << "\n";
        return;
    }

    // Data models
    if (kb.contains("dataModels")) {
        for (auto& [modelName, modelData] : kb["dataModels"].items()) {
            DataModel dm;
            dm.name        = modelName;
            dm.description = modelData.value("description", "");
            if (modelData.contains("sizes")) {
                for (auto& [typeName, bits] : modelData["sizes"].items())
                    dm.sizes[typeName] = bits.get<unsigned>();
            }
            sharedKnowledgeBase.dataModels[modelName] = std::move(dm);
        }
    }

    // Rules
    if (kb.contains("rules")) {
        auto readStrings = [](const json& node, const std::string& key,
                              std::vector<std::string>& out) {
            if (node.contains(key) && node[key].is_array())
                for (auto& v : node[key]) out.push_back(v.get<std::string>());
        };

        for (auto& r : kb["rules"]) {
            TypeSizeRule rule;
            rule.note       = r.value("note", "");
            rule.dataModel  = r.value("dataModel", "");
            readStrings(r, "flags",         rule.flags);
            readStrings(r, "arch",          rule.arch);
            readStrings(r, "os",            rule.os);
            readStrings(r, "compiler",      rule.compiler);
            readStrings(r, "tripleContains",rule.tripleContains);
            sharedKnowledgeBase.rules.push_back(std::move(rule));
        }
    }

    sharedKnowledgeBase.loaded = true;
    std::cout << "[KB] loaded " << sharedKnowledgeBase.dataModels.size()
              << " data models, " << sharedKnowledgeBase.rules.size()
              << " rules from " << path << "\n";
}

// ── Environment → data model resolution ──────────────────────────────────────

std::string resolveDataModel(const ExecutionEnv& env) {
    if (!sharedKnowledgeBase.loaded) return "";

    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    std::string archLow     = toLower(env.arch);
    std::string osLow       = toLower(env.os);
    std::string compLow     = toLower(env.compiler);
    std::string tripleLow   = toLower(env.triple);

    auto containsAny = [](const std::string& haystack,
                          const std::vector<std::string>& needles) {
        for (const auto& n : needles)
            if (haystack.find(n) != std::string::npos) return true;
        return false;
    };

    auto matchesAny = [](const std::string& value,
                         const std::vector<std::string>& list) {
        for (const auto& item : list)
            if (value == item) return true;
        return false;
    };

    for (const auto& rule : sharedKnowledgeBase.rules) {
        // All non-empty condition lists must match (AND logic within a rule)
        bool flagMatch     = rule.flags.empty()         ||
            [&]{ for (auto& f : rule.flags) for (auto& ef : env.flags)
                     if (toLower(ef) == f) return true; return false; }();
        bool archMatch     = rule.arch.empty()          || matchesAny(archLow,   rule.arch);
        bool osMatch       = rule.os.empty()            || matchesAny(osLow,     rule.os);
        bool compMatch     = rule.compiler.empty()      || matchesAny(compLow,   rule.compiler);
        bool tripleMatch   = rule.tripleContains.empty()|| containsAny(tripleLow,rule.tripleContains);

        if (flagMatch && archMatch && osMatch && compMatch && tripleMatch)
            return rule.dataModel;
    }

    return ""; // no rule matched
}

// ── Type bit-width query ──────────────────────────────────────────────────────

unsigned getBitWidthForType(clang::QualType ty, const ExecutionEnv& env) {
    // No env specified: let Clang's ASTContext answer based on the host target.
    if (env.isEmpty())
        return (unsigned)sharedASTContext->getTypeSize(ty);

    // Resolve the data model from the KB.
    std::string modelName = resolveDataModel(env);
    if (!modelName.empty()) {
        auto it = sharedKnowledgeBase.dataModels.find(modelName);
        if (it != sharedKnowledgeBase.dataModels.end()) {
            const auto& sizes = it->second.sizes;

            ty = ty.getCanonicalType();
            if (const auto* bt = dyn_cast<clang::BuiltinType>(ty.getTypePtr())) {
                switch (bt->getKind()) {
                    case clang::BuiltinType::Char_S:
                    case clang::BuiltinType::Char_U:
                    case clang::BuiltinType::SChar:
                    case clang::BuiltinType::UChar:
                        if (sizes.count("char"))     return sizes.at("char");
                        break;
                    case clang::BuiltinType::Short:
                    case clang::BuiltinType::UShort:
                        if (sizes.count("short"))    return sizes.at("short");
                        break;
                    case clang::BuiltinType::Int:
                    case clang::BuiltinType::UInt:
                        if (sizes.count("int"))      return sizes.at("int");
                        break;
                    case clang::BuiltinType::Long:
                    case clang::BuiltinType::ULong:
                        if (sizes.count("long"))     return sizes.at("long");
                        break;
                    case clang::BuiltinType::LongLong:
                    case clang::BuiltinType::ULongLong:
                        if (sizes.count("longlong")) return sizes.at("longlong");
                        break;
                    default: break;
                }
            }
        }
    }

    // Fallback: Clang ASTContext for any type not covered by the data model.
    return (unsigned)sharedASTContext->getTypeSize(ty);
}

#include "Analysis.cpp"

unsigned int sourceLineOfTargetStmt;
string       targetJsonFile;

unsigned int getTargetInstructionSourceLine(string targetJsonFile) {
    std::ifstream f(targetJsonFile);
    json data = json::parse(f);

    int targetInstruction = data["target"].get<int>();
    cout << "target instruction : " << targetInstruction << "\n";

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

#include "ConditionPathAstVisitor.cpp"
#include "ConditionPathAstConsumer.cpp"
#include "AggregateASTConsumer.cpp"

using namespace llvm;

// Locate KnowledgeBase.json: try next to the binary first, then CWD.
static std::string findKnowledgeBasePath(const char* argv0) {
    std::string binPath(argv0);
    auto sep = binPath.find_last_of("/\\");
    std::string dir = (sep != std::string::npos) ? binPath.substr(0, sep + 1) : "./";
    std::string candidate = dir + "KnowledgeBase.json";
    if (std::ifstream(candidate).good()) return candidate;
    if (std::ifstream("KnowledgeBase.json").good()) return "KnowledgeBase.json";
    return "";
}

int main(int argc, const char **argv) {
    // Make stdout unbuffered so output order is consistent whether or not
    // stdout is connected to a TTY (e.g. inside Docker).
    std::cout << std::unitbuf;

    // Load the knowledge base before parsing the target JSON.
    std::string kbPath = findKnowledgeBasePath(argv[0]);
    if (!kbPath.empty())
        loadKnowledgeBase(kbPath);

    if (argc >= 3) {
        string fullJsonPath = argv[2];
        targetJsonFile = fullJsonPath.substr(fullJsonPath.find_last_of("/\\") + 1);
        cout << "file name : " << targetJsonFile << "\n";
        sourceLineOfTargetStmt = getTargetInstructionSourceLine(fullJsonPath);
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

    {
        HeaderSearchOptions &HSO = CI.getHeaderSearchOpts();
        HSO.ResourceDir = LLVM_PREFIX "/lib/clang/" CLANG_VERSION_STRING;
        // Add Clang's own built-in headers (stddef.h, stdint.h, …) explicitly.
        // Without this, Clang's manual CompilerInstance setup does not add the
        // resource-dir include subdir to the search path automatically.
        HSO.AddPath(LLVM_PREFIX "/lib/clang/" CLANG_VERSION_STRING "/include",
                    clang::frontend::System, false, false);
        HSO.AddPath("/usr/local/include",             clang::frontend::System, false, false);
        HSO.AddPath("/usr/include/x86_64-linux-gnu",  clang::frontend::System, false, false);
        HSO.AddPath("/usr/include",                   clang::frontend::System, false, false);
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
