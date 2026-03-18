/**
 * Unit tests for pure-logic functions that do not require a live Clang AST.
 *
 * Coverage:
 *   ExecutionEnv::isEmpty()            — struct predicate
 *   SSAVariable::ssaName()             — SSA name formatting
 *   getCurrentSsaVersionOfVariable()   — global varDefs lookup
 *   TargetProgramPoint::getBaseNameFromSSA()  — SSA name → base name
 *   rewriteWithSSA()                   — condition-string rewriting
 *
 * Functions that require a live Clang Expr* / Stmt* (exprToPyZ3,
 * handleArithmeticWithOverflow, visitAllStmts, VisitBinaryOperator, ...)
 * are covered only by the integration tests in run_tests.sh.
 */

#include "main.h"

// Definitions of globals declared extern in main.h
ASTContext *sharedASTContext = nullptr;
ExecutionEnv sharedExecutionEnv;
std::vector<UserConstraint> sharedConstraints;
KnowledgeBase sharedKnowledgeBase;

// Globals defined in main.cpp that Analysis.cpp depends on indirectly
std::string functionNameDump = "";

#include "Analysis.cpp"

// ── Minimal test framework ──────────────────────────────────────────────────

static int g_run = 0, g_pass = 0, g_fail = 0;
static std::string g_current_suite;

static void suite(const std::string& name) {
    g_current_suite = name;
    std::cout << "\n[" << name << "]\n";
}

#define CHECK_EQ(got, expected) do {                                          \
    g_run++;                                                                  \
    if ((got) == (expected)) {                                                \
        g_pass++;                                                             \
        std::cout << "  PASS  " << #got << "\n";                             \
    } else {                                                                  \
        g_fail++;                                                             \
        std::cerr << "  FAIL  " << #got                                       \
                  << "\n         expected: [" << (expected) << "]"           \
                  << "\n         got:      [" << (got) << "]\n";             \
    }                                                                         \
} while(0)

#define CHECK_TRUE(expr) do {                                                 \
    g_run++;                                                                  \
    if (expr) { g_pass++; std::cout << "  PASS  " << #expr << "\n"; }       \
    else { g_fail++; std::cerr << "  FAIL  " << #expr << "\n"; }            \
} while(0)

#define CHECK_FALSE(expr) CHECK_TRUE(!(expr))

// ── Helpers ─────────────────────────────────────────────────────────────────

// Insert a variable into the global varDefs map directly so tests can
// exercise getCurrentSsaVersionOfVariable / findLatestSSAName / rewriteWithSSA
// without needing a Clang AST.
static void registerVar(const std::string& baseName, int version,
                        const std::string& smtExpr = "") {
    SSAVariable v(baseName, version);
    DefinitionInfo info(v, /*line=*/1, /*stmtID=*/0, /*type=*/"int");
    info.isCondtionalDefinition = false;
    info.smtDefinitionExpression = smtExpr;
    varDefs[v.ssaName()] = info;
}

static void clearVarDefs() { varDefs.clear(); }

// ── Test suites ──────────────────────────────────────────────────────────────

void test_isEmpty() {
    suite("ExecutionEnv::isEmpty()");

    ExecutionEnv e;
    CHECK_TRUE(e.isEmpty());

    e.arch = "x86_64";
    CHECK_FALSE(e.isEmpty());

    e = ExecutionEnv{};
    e.os = "linux";
    CHECK_FALSE(e.isEmpty());

    // Regression: flags-only config must NOT be treated as empty.
    // This was the -m32 bug: isEmpty() used to ignore flags.
    e = ExecutionEnv{};
    e.flags.push_back("-m32");
    CHECK_FALSE(e.isEmpty());

    e = ExecutionEnv{};
    e.triple = "x86_64-pc-linux-gnu";
    CHECK_FALSE(e.isEmpty());
}

void test_ssaName() {
    suite("SSAVariable::ssaName()");

    SSAVariable v1("x", 0);
    CHECK_EQ(v1.ssaName(), "x_0");

    SSAVariable v2("uid_sid", 3);
    CHECK_EQ(v2.ssaName(), "uid_sid_3");

    SSAVariable v3("result", 12);
    CHECK_EQ(v3.ssaName(), "result_12");
}

void test_getCurrentVersion() {
    suite("getCurrentSsaVersionOfVariable()");
    clearVarDefs();

    // Unknown variable → -1
    CHECK_EQ(getCurrentSsaVersionOfVariable("x"), -1);

    registerVar("x", 0);
    CHECK_EQ(getCurrentSsaVersionOfVariable("x"), 0);

    registerVar("x", 1);
    registerVar("x", 2);
    CHECK_EQ(getCurrentSsaVersionOfVariable("x"), 2);

    // Other variables do not interfere
    registerVar("y", 0);
    CHECK_EQ(getCurrentSsaVersionOfVariable("x"), 2);
    CHECK_EQ(getCurrentSsaVersionOfVariable("y"), 0);

    clearVarDefs();
}

void test_getBaseNameFromSSA() {
    suite("getBaseNameFromSSA()");

    CHECK_EQ(getBaseNameFromSSA("x_0"),        "x");
    CHECK_EQ(getBaseNameFromSSA("uid_sid_3"),  "uid_sid");
    CHECK_EQ(getBaseNameFromSSA("result_12"),  "result");

    // No version suffix → returned as-is
    CHECK_EQ(getBaseNameFromSSA("noversion"),  "noversion");

    // Empty string → returned as-is
    CHECK_EQ(getBaseNameFromSSA(""),           "");

    // Trailing underscore with no digits → returned as-is
    CHECK_EQ(getBaseNameFromSSA("bad_"),       "bad_");
}

void test_rewriteWithSSA() {
    suite("rewriteWithSSA()");
    clearVarDefs();

    registerVar("a", 0);
    registerVar("b", 2);
    registerVar("uid_sid", 1);

    // Single variable replacement
    CHECK_EQ(rewriteWithSSA("a > 0", {"a_0"}), "a_0 > 0");

    // Two variables in one expression
    CHECK_EQ(rewriteWithSSA("a + b", {"a_0", "b_2"}), "a_0 + b_2");

    // No relevant variables → unchanged
    CHECK_EQ(rewriteWithSSA("a + b", {}), "a + b");

    // Multi-word base name rewritten correctly
    CHECK_EQ(rewriteWithSSA("uid_sid == 0", {"uid_sid_1"}), "uid_sid_1 == 0");

    clearVarDefs();
}

// ── Entry point ─────────────────────────────────────────────────────────────

int main() {
    test_isEmpty();
    test_ssaName();
    test_getCurrentVersion();
    test_getBaseNameFromSSA();
    test_rewriteWithSSA();

    std::cout << "\n─────────────────────────────────────\n";
    std::cout << "Results: " << g_pass << " passed, "
              << g_fail  << " failed  (total " << g_run << ")\n";

    return g_fail > 0 ? 1 : 0;
}
