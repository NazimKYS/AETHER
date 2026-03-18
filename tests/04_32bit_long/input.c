// Test: on a 32-bit architecture (ILP32/LLP64), 'long' is 32 bits wide.
// Overflow on long addition can produce a negative result.
// The -m32 flag in executionEnv.flags must shrink the type range constraint
// to [-2^31, 2^31-1] instead of the 64-bit default.
// Expected Z3 result: sat  (overflow is possible with large positive inputs)

void flag_negative(void);

void add_longs(long a, long b) {
    long result = a + b;
    if (result < 0) {
        flag_negative();
    }
}
