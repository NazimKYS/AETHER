// Test: the nested conditions x > 100 AND x < 50 are mutually exclusive.
// The inner body can never be reached.
// Expected Z3 result: unsat

void unreachable_code(void);

void check(int x) {
    if (x > 100) {
        if (x < 50) {
            unreachable_code();
        }
    }
}
