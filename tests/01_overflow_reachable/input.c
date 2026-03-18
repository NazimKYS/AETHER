// Test: integer overflow on addition can produce a negative sum,
// making the "sum < 0" branch reachable even when both inputs are positive.
// Expected Z3 result: sat

void report_overflow(void);

void check(int a, int b) {
    int sum = a + b;
    if (sum < 0) {
        report_overflow();
    }
}
