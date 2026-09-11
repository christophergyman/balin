#include <assert.h>
#include <stdio.h>

// Scaffold smoke test. Real tests land with the engine modules.
static void TestScaffold(void) {
    assert(1 == 1);
}

int main(void) {
    TestScaffold();

    printf("balin_tests: all tests passed\n");
    return 0;
}
