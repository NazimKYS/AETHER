# AETHER — Architecture-aware Execution and THeoretical Reasoning

**AETHER** is a path-sensitive static analysis tool for C programs.
Given a target line in a function, it computes the exact conditions required to reach that line, models all relevant integer arithmetic including overflow, and discharges the resulting constraints to the **Z3 SMT solver** to determine whether the path is reachable — and if so, **what concrete input values witness the reachability**.

---

## What problem does it solve?

Consider an access-control check like:

```c
long int uid_sid = (long)userId * serviceId;
if (uid_sid == 0) {
    readAndWriteService(uid_sid, serviceId);   // ← privileged branch
}
```

A code reviewer may assume this branch is dead when `userId != 0` and `serviceId > 0`.
AETHER proves this assumption wrong: on a 32-bit architecture (Windows, `-m32`, x86/Intel/AMD), the multiplication can **overflow** and wrap to zero, silently granting write access.
Z3 returns the exact witness: `userId = 134217728`, `serviceId = 64` → product overflows to `0`.

---

## Requirements

| Method | Requirements |
|---|---|
| **Docker** (recommended) | Docker Desktop or Docker Engine |
| **From source** | Ubuntu 22.04, LLVM/Clang 18, Python 3 + z3-solver, g++, make |

---

## Quick start — Docker

### 1. Pull the image

```bash
docker pull nazimkys/aether:latest
```

### 2. Analyse your first program

```bash
docker run --rm \
  -v $(pwd):/work \
  nazimkys/aether \
  /work/my_program.c /work/target.json
```

The tool reads both files from `/work/` (your current directory mounted into the container) and prints the Z3 result directly to the terminal.

---

## Complete walkthrough

### The C program — `samples/pseudo.c`

```c
void readAndWriteService(int a, int b) {}
void readOnlyService(int a, int b) {}

void foo(char username[], char password[]) {
    int userId   = random_int(0, 150000000); // getUserId(username,password)
    int serviceId = random_int(1, 64);
    long int uid_sid = (long)userId * serviceId;

    if (uid_sid == 0) {
        readAndWriteService(uid_sid, serviceId);   // line 26 — privileged
    } else {
        readOnlyService(uid_sid, serviceId);
    }
}
```

**Question:** Can `readAndWriteService` (line 26) ever be called when `userId != 0` and `serviceId > 0`?

---

### The target file — `target.json`

`target.json` is the single entry point that tells AETHER **what to analyse** and **under what conditions**.

```json
{
  "target": 26,
  "constraints": [
    { "variable": "userId",    "operator": ">=", "value": 0         },
    { "variable": "userId",    "operator": "<",  "value": 150000000 },
    { "variable": "userId",    "operator": "!=", "value": 0         },
    { "variable": "serviceId", "operator": ">",  "value": 0         },
    { "variable": "serviceId", "operator": "<",  "value": 65        }
  ],
  "executionEnv": {
    "Arch":     "x86",
    "OS":       "windows",
    "compiler": "",
    "flags":    [],
    "triple":   ""
  }
}
```

#### Field reference

| Field | Type | Description |
|---|---|---|
| `target` | `int` | **Source line number** of the statement to analyse |
| `constraints` | `array` | User-supplied safety assumptions about variable ranges |
| `constraints[].variable` | `string` | Base variable name (no SSA suffix) |
| `constraints[].operator` | `string` | One of: `==` `!=` `<` `<=` `>` `>=` |
| `constraints[].value` | `number` | Numeric bound |
| `executionEnv` | `object` | Describes the target platform (affects integer type widths) |
| `executionEnv.Arch` | `string` | Architecture: `x86_64`, `x86`, `arm`, `aarch64`, `riscv64`, `avr`, … |
| `executionEnv.OS` | `string` | OS: `linux`, `windows`, `macos`, `darwin`, … |
| `executionEnv.compiler` | `string` | Compiler: `gcc`, `clang`, `msvc`, `mingw`, … |
| `executionEnv.flags` | `array` | Compiler flags that affect type widths: `["-m32"]`, `["-m64"]` |
| `executionEnv.triple` | `string` | Full target triple e.g. `x86_64-pc-linux-gnu` |

> **Tip:** All `executionEnv` fields are optional. Leave them empty (`""` / `[]`) to use the host architecture's type widths. Fill them when analysing code compiled for a **different** target (e.g. a Windows binary from Linux, or an embedded MCU).

---

### Running the tool

#### With Docker

```bash
# From the repository root (where target.json lives)
docker run --rm \
  -v $(pwd):/work \
  nazimkys/aether \
  /work/samples/pseudo.c /work/target.json
```

#### Built from source (local)

```bash
./aether ./samples/pseudo.c ./target.json
```

---

### Terminal output

```
[KB] loaded 4 data models, 18 rules from ./KnowledgeBase.json
file name : target.json
target instruction : 26

TARGET FOUND: CallExpr at line 26

=== SMT Context ===
Target path conditions:
  uid_sid_0 == 0
Relevant variables:  serviceId_0  uid_sid_0  userId_0

[*] Script written to checking.py

 ----- Durations -----
 AST build    :  5.3 ms
 script gen   :  2.6 ms
 Z3 checking  :  788 ms

sat
[serviceId_0 = 64, xbar1 = 0, userId_0 = 134217728, k1 = 2]
```

---

### Understanding the result

**`sat`** means the path to line 26 IS reachable under the given constraints.

The model gives you the **concrete witness**:

| Variable | Value | Meaning |
|---|---|---|
| `userId_0` | `134217728` | `userId = 2²⁷` |
| `serviceId_0` | `64` | `serviceId = 64` |
| `k1` | `2` | the product wrapped around **2 times** |
| `xbar1` | `0` | the overflow residue = **0** |

Verification: `134217728 × 64 = 8589934592 = 2 × 2³² + 0` → stored as `0` in a 32-bit `long` on Windows/x86.

**Conclusion:** A caller with `userId = 134217728` and `serviceId = 64` bypasses the zero-check and gains write access — a real **integer overflow vulnerability**.

---

### When the result is `unsat`

```c
void check(int x) {
    if (x > 100) {
        if (x < 50) {
            unreachable_code();   // ← target line
        }
    }
}
```

`target.json`:
```json
{ "target": 4,  "constraints": [] }
```

Result:
```
unsat
```

`unsat` means **no possible input** can reach the target. The path `x > 100 AND x < 50` is contradictory — the dead-code branch is formally proven unreachable.

---

### Generated Z3 script — `checking.py`

AETHER writes `checking.py` to the current directory. You can inspect it, modify it, or run it independently:

```bash
python3 checking.py
```

Structure of the generated script:

```python
from z3 import *

# *** Variables Declarations ***
serviceId_0 = Int('serviceId_0')
uid_sid_0   = Int('uid_sid_0')
userId_0    = Int('userId_0')
k1          = Int('k1')       # overflow quotient
xbar1       = Int('xbar1')    # overflow residue

# *** Variables Definitions ***
# Overflow-aware definition: if userId_0 * serviceId_0 overflows (k1 > 0),
# the stored value is the residue xbar1, otherwise the exact product.
uid_sid_0 = If(
    And((userId_0 * serviceId_0) == 2**32 * k1 + xbar1, k1 > 0),
    xbar1,
    (userId_0 * serviceId_0)
)

# *** Condition path ***
s = Solver()
s.add(uid_sid_0 == 0)        # condition to reach line 26

# *** User constraints ***
s.add(userId_0 >= 0)
s.add(userId_0 < 150000000)
s.add(userId_0 != 0)
s.add(serviceId_0 > 0)
s.add(serviceId_0 < 65)

# *** Type range constraints (from KnowledgeBase — LLP64 for Windows x86) ***
s.add(serviceId_0 >= -2**31);  s.add(serviceId_0 <= 2**31 - 1)
s.add(uid_sid_0   >= -2**31);  s.add(uid_sid_0   <= 2**31 - 1)
s.add(userId_0    >= -2**31);  s.add(userId_0    <= 2**31 - 1)

print(s.check())
if s.check() == sat:
    print(s.model())
```

---

## Architecture-aware type widths — `KnowledgeBase.json`

AETHER ships with a knowledge base that maps execution environments to **C data models**. Integer type widths vary across platforms — the `long` type is 32 bits on Windows but 64 bits on Linux/macOS 64-bit. Getting this wrong would model overflow at the wrong boundary, producing incorrect results.

| Data model | `long` | `int` | Applies to |
|---|---|---|---|
| **LP64**  | 64 bits | 32 bits | Linux / macOS 64-bit |
| **LLP64** | 32 bits | 32 bits | Windows (MSVC, MinGW) |
| **ILP32** | 32 bits | 32 bits | 32-bit architectures, `-m32` |
| **ILP16** | 32 bits | 16 bits | AVR, MSP430 embedded MCUs |

The correct data model is selected automatically from your `executionEnv` specification. Type range constraints in the generated script (`-2³¹ … 2³¹ - 1` vs `-2⁶³ … 2⁶³ - 1` for `long`) are set accordingly.

You can extend `KnowledgeBase.json` to add custom architectures or compilers without recompiling the tool.

---

## Build from source

### Prerequisites

```bash
# Ubuntu 22.04
sudo apt-get install -y \
  g++ make nlohmann-json3-dev \
  llvm-18-dev libclang-18-dev libclang-cpp18-dev clang-18 \
  libtinfo-dev zlib1g-dev

pip3 install z3-solver
```

### Build

```bash
git clone https://github.com/NazimKYS/aether.git
cd aether
make
```

### Run tests

```bash
make test          # unit tests + integration tests
make unit-test     # unit tests only
bash tests/run_tests.sh   # integration tests only
```

---

## Repository structure

```
aether/
├── src/
│   ├── main.cpp                  # entry point, KB loading, getBitWidthForType
│   ├── main.h                    # shared types (ExecutionEnv, KnowledgeBase, …)
│   ├── Analysis.cpp              # SSA construction, path conditions, SMT generation
│   ├── ConditionPathAstVisitor.cpp   # Clang AST visitor (finds target statement)
│   ├── ConditionPathAstConsumer.cpp  # AST consumer (drives analysis, writes script)
│   └── AggregateASTConsumer.cpp  # plumbing
├── tests/
│   ├── unit_tests.cpp            # unit tests (no Clang AST required)
│   ├── run_tests.sh              # integration test runner
│   ├── 01_overflow_reachable/    # sat: int overflow makes branch reachable
│   ├── 02_unreachable_path/      # unsat: x > 100 AND x < 50
│   ├── 03_simple_path/           # sat: basic conditional
│   └── 04_32bit_long/            # sat: long overflow with -m32
├── samples/                      # example C programs
├── KnowledgeBase.json            # type-size rules per architecture/OS/compiler
├── target.json                   # example target configuration
├── Makefile
└── Dockerfile
```

---

## How it works — technical pipeline

```
C source file                    target.json
      │                               │
      ▼                               ▼
 Clang AST parser          reads target line,
      │                    user constraints, execution env
      ▼
 Path-condition extraction
 (walks the AST, collects all
  if/else conditions on every
  path from function entry to
  the target line)
      │
      ▼
 SSA construction
 (assigns unique version numbers
  to every variable definition:
  userId_0, userId_1, …)
      │
      ▼
 SMT script generation  ──────────►  checking.py
 (encodes overflow with fresh         (pure Python / Z3)
  variables k, xbar per operation)
      │
      ▼
 python3 checking.py
      │
      ▼
 sat  →  concrete witness printed     (path IS reachable)
 unsat →  no output                   (path is NOT reachable)
```

---

## Theoretical Foundation

AETHER implements the formal safety-verification approach described in:

> S.Y. Kissi, R. Ameur-Boulifa, Y. Seladji — *Identifying Security Vulnerabilities in Source Code with Safety Verification* — published in *International Journal of Critical Computer-Based Systems* ([forthcoming](https://www.inderscience.com/info/ingeneral/forthcoming.php?jcode=ijccbs))

The central question AETHER answers is:

> **Does there exist an execution context `EC` and program inputs that satisfy the path condition `PC` to reach the target line, while violating the user-supplied security constraint `SC`?**

Formally:

```
EC  ⊢  PC  ∧  ¬SC
```

- **PC** (Path Condition) — conjunction of all branch conditions on the path from function entry to the target line, computed by `computeCondPath(P, ℓ)`.
- **SC** (Security Constraint) — the user-supplied safety assumption (the `constraints` array in `target.json`). AETHER tests its negation `¬SC`.
- **EC** (Execution Context) — the target architecture, OS, compiler, and flags (`executionEnv`), used to determine integer type widths.

If the formula is **satisfiable**, Z3 returns a concrete counterexample. If it is **unsatisfiable**, the path is formally proven unreachable under those constraints.

### Overflow encoding

Machine integers overflow silently. For a binary expression `exp` producing an `n`-bit result, the overflow is encoded with two fresh variables per operation:

```
(x̄  =  2ⁿ × k  +  exp)  ∧  (k ≠ 0)
```

where `k` is the overflow quotient and `x̄` (`xbar`) is the residue stored in the variable. The `encodeArch` function produces the conditional structure:

```
ite(k > 0,  x̄,  exp)
```

meaning: *if overflow occurred, the stored value is `x̄`; otherwise it is `exp` itself.* The bit width `n` is resolved from the KnowledgeBase using the `executionEnv`. In `checking.py` this appears as the `If(And(..., k1 > 0), xbar1, ...)` expression shown above.

### The three analysis phases

| Phase | What happens |
|---|---|
| **1 — Program Analysis** | Clang AST traversal; path conditions collected; SSA form constructed; relevant variables identified |
| **2 — Model Construction** | `FExec = PC ∧ ¬SC`; `FEnv = type-range constraints ∧ overflow-aware definitions`; combined formula: `FExec ∧ FEnv` |
| **3 — Checking Satisfiability** | `checking.py` written; `python3 checking.py` invoked; `sat` → witness, `unsat` → proven safe |

---

## Limitations

- **No loop support yet** — loops are not unrolled; variables assigned inside loops may not be tracked correctly.
- **Function calls as symbolic inputs** — when a variable is assigned from a function call (`int x = foo()`), `x` is treated as a free (unconstrained) symbolic variable. This is sound but may produce spurious `sat` results if the callee has a restricted range.
- **Single function scope** — analysis is intra-procedural: only the function that contains the target line is analysed.

---

## Licence

MIT
