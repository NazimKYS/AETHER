// Test: basic conditional path reachability.
// status = code + code, so status == 0 iff code == 0.
// The if-branch is reachable (code = 0 is a valid witness).
// Expected Z3 result: sat

void handle(int s);

void process(int code) {
    int status = code + code;
    if (status == 0) {
        handle(status);
    }
}
