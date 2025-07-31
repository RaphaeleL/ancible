#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Forward declarations of functions to test
extern void print_usage(const char *program_name);

/**
 * Simple test harness for main.c
 */
int main(void) {
    printf("Running tests for main.c\n");
    
    // Test print_usage function
    // We can't easily test the output directly, but we can ensure it doesn't crash
    printf("Testing print_usage()... ");
    print_usage("test-program");
    printf("OK\n");
    
    printf("All tests passed!\n");
    return 0;
}
